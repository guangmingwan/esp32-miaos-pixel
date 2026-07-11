#include <stdint.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern int mia_emulator_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);
typedef struct __attribute__((packed)) { uint32_t seq; uint8_t label[20]; uint32_t state; uint32_t crc; } OtaEntry;

static void set_entry(OtaEntry *entry, uint32_t seq) {
    memset(entry, 0, sizeof(*entry)); entry->seq = seq; entry->state = 2;
    entry->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&entry->seq, 4);
}
static esp_err_t return_launcher(void) {
    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (part == NULL) return ESP_ERR_NOT_FOUND;
    OtaEntry entries[2]; set_entry(&entries[0], 1); set_entry(&entries[1], 3);
    esp_err_t error = esp_partition_erase_range(part, 0, part->size);
    if (error == ESP_OK) error = esp_partition_write(part, 0, &entries[0], sizeof(entries[0]));
    if (error == ESP_OK) error = esp_partition_write(part, 4096, &entries[1], sizeof(entries[1]));
    return error;
}
static void emulator_task(void *unused) {
    (void)unused; char *argv[] = {"megadrive"};
    (void)mia_emulator_main_impl(1, argv);
    const esp_err_t error = return_launcher();
    if (error == ESP_OK) esp_restart();
    ESP_LOGE("megadrive", "launcher return failed: %s", esp_err_to_name(error));
    vTaskDelete(NULL);
}
void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    if (xTaskCreatePinnedToCore(emulator_task, "megadrive", 32768, NULL, 5, NULL, 1) != pdPASS)
        ESP_LOGE("megadrive", "emulator task allocation failed");
}
