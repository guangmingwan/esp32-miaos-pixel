#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "display_host.h"
#include "lava/lava_rt.h"
#include "screen_config.h"

extern char ExePath[260];
extern int lava_cch_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);

typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
} OtaEntry;

static void set_entry(OtaEntry *entry, uint32_t sequence, uint32_t state)
{
    memset(entry, 0, sizeof(*entry));
    entry->ota_seq = sequence;
    entry->ota_state = state;
    entry->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&entry->ota_seq, 4);
}

static void switch_to_launcher(void)
{
    const esp_partition_t *otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    OtaEntry entries[2];

    if (otadata == NULL)
        return;
    set_entry(&entries[0], 1, ESP_OTA_IMG_VALID);
    set_entry(&entries[1], 3, ESP_OTA_IMG_VALID);
    if (esp_partition_erase_range(otadata, 0, otadata->size) != ESP_OK)
        return;
    if (esp_partition_write(otadata, 0, &entries[0], sizeof(entries[0])) != ESP_OK)
        return;
    (void)esp_partition_write(otadata, 4096, &entries[1], sizeof(entries[1]));
}

static int select_game_directory(void)
{
    static const char *const candidates[] = {
        "/sd/Games/lava_cch.app",
        "/sd/MiaOS/Games/lava_cch.app",
        "/sd/lava_cch",
    };
    char path[320];

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        FILE *probe;

        snprintf(path, sizeof(path), "%s/LavaData/BOOK.DAT", candidates[i]);
        probe = fopen(path, "rb");
        if (probe == NULL) {
            ESP_LOGW("lava_cch", "missing %s: errno=%d", path, errno);
            continue;
        }
        fclose(probe);
        strncpy(ExePath, candidates[i], sizeof(ExePath) - 1);
        ExePath[sizeof(ExePath) - 1] = '\0';
        ESP_LOGI("lava_cch", "using game data at %s", ExePath);
        return 1;
    }
    return 0;
}

static void game_task(void *argument)
{
    char *argv[] = {"lava_cch"};
    LavaRuntime *runtime = NULL;
    (void)argument;

    if (!select_game_directory()) {
        ESP_LOGE("lava_cch", "LavaData/BOOK.DAT not found on SD card");
    } else {
        runtime = lrt_create(LAVA_CCH_SCREEN_WIDTH, LAVA_CCH_SCREEN_HEIGHT, NULL);
        if (runtime == NULL) {
            ESP_LOGE("lava_cch", "failed to create Lava runtime");
        } else {
            (void)lrt_set_graph_mode(4);
            (void)lava_cch_main_impl(1, argv);
            lrt_destroy(runtime);
        }
    }

    switch_to_launcher();
    esp_restart();
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(host_platform_init());
    display_host_backlight_set(0);
    display_host_fill_screen_rgb565(0);
    display_host_backlight_set(1);
    xTaskCreatePinnedToCore(game_task, "lava_cch", 65536, NULL, 5, NULL, 1);
}
