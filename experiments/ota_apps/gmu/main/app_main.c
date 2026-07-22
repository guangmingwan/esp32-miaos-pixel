#include "mia_host_abi.h"
#include "launch_context.h"

#include <esp_err.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdio.h>
#include <string.h>

extern int gmu_main(int argc, char **argv);
extern esp_err_t host_platform_init(void);
extern void gmu_frontend_set_autoplay(uint8_t enabled);

static const char *TAG = "gmu_app";

typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
} OtaEntry;

static void set_ota_entry(OtaEntry *entry, uint32_t seq) {
    memset(entry, 0, sizeof(*entry));
    entry->ota_seq = seq;
    entry->ota_state = ESP_OTA_IMG_VALID;
    entry->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&entry->ota_seq, 4);
}

static void switch_to_launcher(void) {
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (partition == NULL) return;
    OtaEntry entries[2];
    set_ota_entry(&entries[0], 1);
    set_ota_entry(&entries[1], 3);
    if (esp_partition_erase_range(partition, 0, partition->size) != ESP_OK) return;
    if (esp_partition_write(partition, 0, &entries[0], sizeof(entries[0])) != ESP_OK) return;
    esp_partition_write(partition, 4096, &entries[1], sizeof(entries[1]));
}

static void gmu_task(void *arg) {
    (void)arg;
    char launch_path[MIA_HOST_LAUNCH_ARG_SIZE];
    char *argv[2] = {"gmu", NULL};
    if (mia_host_consume_launch_arg("gmu", launch_path, sizeof(launch_path))) {
        static char direct_path[sizeof(launch_path) + 5];
        if (strncmp(launch_path, "/sd/", 4) == 0)
            snprintf(direct_path, sizeof(direct_path), "%s", launch_path);
        else
            snprintf(direct_path, sizeof(direct_path), "/sd%s", launch_path);
        argv[1] = direct_path;
    }
    gmu_frontend_set_autoplay(argv[1] != NULL);
    gmu_main(argv[1] == NULL ? 1 : 2, argv);
    switch_to_launcher();
    esp_restart();
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    BaseType_t result = xTaskCreatePinnedToCore(gmu_task, "gmu_main", 16384,
                                                NULL, 5, NULL, 1);
    ESP_LOGI(TAG, "gmu task create => %s", result == pdPASS ? "ok" : "failed");
}
