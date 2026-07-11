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

extern int mia_emulator_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);

#ifndef MIA_APP_NAME
#error "MIA_APP_NAME is required"
#endif

static const char *TAG = MIA_APP_NAME "_app";

typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
} MiaOtaEntry;

static void set_entry(MiaOtaEntry *entry, uint32_t seq, uint32_t state) {
    memset(entry, 0, sizeof(MiaOtaEntry));
    entry->ota_seq = seq;
    entry->ota_state = state;
    entry->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&entry->ota_seq, 4);
}

static esp_err_t switch_to_launcher(void) {
    const esp_partition_t *otap = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (otap == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    MiaOtaEntry entries[2];
    set_entry(&entries[0], 1, ESP_OTA_IMG_VALID);
    set_entry(&entries[1], 3, ESP_OTA_IMG_VALID);
    esp_err_t err = esp_partition_erase_range(otap, 0, otap->size);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_partition_write(otap, 0, &entries[0], sizeof(MiaOtaEntry));
    if (err != ESP_OK) {
        return err;
    }
    return esp_partition_write(otap, 4096, &entries[1], sizeof(MiaOtaEntry));
}

static void emulator_task(void *arg) {
    char *argv[] = {MIA_APP_NAME};
    (void)arg;
    (void)mia_emulator_main_impl(1, argv);
    const esp_err_t err = switch_to_launcher();
    if (err == ESP_OK) {
        esp_restart();
    }
    ESP_LOGE(TAG, "launcher return failed: %s", esp_err_to_name(err));
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    xTaskCreatePinnedToCore(emulator_task, MIA_APP_NAME "_emu", 32768, NULL, 5, NULL, 1);
}
