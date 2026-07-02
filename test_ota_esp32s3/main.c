#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "driver/gpio.h"

#define BLINK_GPIO 13
static const char *TAG = "ota-test";

void app_main(void) {
    ESP_LOGI(TAG, "=== OTA TEST FIRMWARE ===");

    // Mark this OTA slot valid immediately
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "mark_valid => %s", esp_err_to_name(err));

    // Print which partition we're running from
    const esp_partition_t *part = esp_ota_get_running_partition();
    if (part) {
        ESP_LOGI(TAG, "Running from: %s @ 0x%08x", part->label, (unsigned)part->address);
    }

    // Turn on backlight for visual feedback
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 1);

    ESP_LOGI(TAG, "OTA test firmware ready!");

    // Keep alive
    int count = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "alive #%d, heap=%u", ++count, (unsigned)esp_get_free_heap_size());
    }
}
