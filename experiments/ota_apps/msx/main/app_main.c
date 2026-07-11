#include <stdint.h>
#include <string.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern int mia_msx_main(void);
extern esp_err_t host_platform_init(void);

typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t label[20];
    uint32_t state;
    uint32_t crc;
} OtaEntry;

static void set_entry(OtaEntry *entry, uint32_t sequence) {
    memset(entry, 0, sizeof(*entry));
    entry->ota_seq = sequence;
    entry->state = ESP_OTA_IMG_VALID;
    entry->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&entry->ota_seq, 4);
}

static esp_err_t select_launcher(void) {
    const esp_partition_t *ota = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (ota == NULL) return ESP_ERR_NOT_FOUND;
    OtaEntry entries[2];
    set_entry(&entries[0], 1);
    set_entry(&entries[1], 3);
    esp_err_t error = esp_partition_erase_range(ota, 0, ota->size);
    if (error == ESP_OK) error = esp_partition_write(ota, 0, &entries[0], sizeof(entries[0]));
    if (error == ESP_OK) error = esp_partition_write(ota, 4096, &entries[1], sizeof(entries[1]));
    return error;
}

static void msx_task(void *unused) {
    (void)unused;
    (void)mia_msx_main();
    const esp_err_t error = select_launcher();
    if (error == ESP_OK) esp_restart();
    ESP_LOGE("msx_app", "launcher return failed: %s", esp_err_to_name(error));
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    if (xTaskCreatePinnedToCore(msx_task, "msx_emu", 65536, NULL, 5, NULL, 1) != pdPASS) {
        ESP_LOGE("msx_app", "emulator task allocation failed");
    }
}
