#include <string.h>

#include <esp_err.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern int screen_test_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);

typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
} OtaEntry;

static void set_entry(OtaEntry *e, uint32_t seq, uint32_t state) {
    memset(e, 0, sizeof(OtaEntry));
    e->ota_seq = seq;
    e->ota_state = state;
    e->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&e->ota_seq, 4);
}

static void switch_to_launcher(void) {
    const esp_partition_t *otap = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (!otap) {
        return;
    }

    OtaEntry entries[2];
    set_entry(&entries[0], 1, ESP_OTA_IMG_VALID);
    set_entry(&entries[1], 3, ESP_OTA_IMG_VALID);

    esp_err_t err = esp_partition_erase_range(otap, 0, otap->size);
    if (err != ESP_OK) return;
    err = esp_partition_write(otap, 0, &entries[0], sizeof(OtaEntry));
    if (err != ESP_OK) return;
    esp_partition_write(otap, 4096, &entries[1], sizeof(OtaEntry));
}

static void ui_task(void *arg) {
    char *argv[] = {"screen_test"};
    (void)arg;

    screen_test_main_impl(1, argv);
    switch_to_launcher();
    esp_restart();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    xTaskCreatePinnedToCore(ui_task, "screen_test_ui", 32768, NULL, 5, NULL, 1);
}
