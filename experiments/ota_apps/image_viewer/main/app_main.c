#include <string.h>

#include <esp_err.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "launch_context.h"

extern int image_viewer_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);

typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
} OtaEntry;

static void set_entry(OtaEntry *entry, uint32_t sequence, uint32_t state) {
    memset(entry, 0, sizeof(*entry));
    entry->ota_seq = sequence;
    entry->ota_state = state;
    entry->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&entry->ota_seq, 4);
}

static void switch_to_launcher(void) {
    const esp_partition_t *otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (otadata == NULL) return;
    OtaEntry entries[2];
    set_entry(&entries[0], 1, ESP_OTA_IMG_VALID);
    set_entry(&entries[1], 3, ESP_OTA_IMG_VALID);
    if (esp_partition_erase_range(otadata, 0, otadata->size) != ESP_OK) return;
    if (esp_partition_write(otadata, 0, &entries[0], sizeof(entries[0])) != ESP_OK) return;
    esp_partition_write(otadata, 4096, &entries[1], sizeof(entries[1]));
}

static void ui_task(void *arg) {
    char direct_path[MIA_HOST_LAUNCH_ARG_SIZE] = {0};
    char *argv[] = {"image_viewer", direct_path};
    int argc = 1;
    (void)arg;
    if (mia_host_consume_launch_arg("image_viewer", direct_path, sizeof(direct_path))) {
        argc = 2;
    }
    image_viewer_main_impl(argc, argv);
    switch_to_launcher();
    esp_restart();
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    xTaskCreatePinnedToCore(ui_task, "image_viewer_ui", 32768, NULL, 5, NULL, 1);
}
