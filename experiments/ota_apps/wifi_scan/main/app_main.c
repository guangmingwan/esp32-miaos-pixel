/*
 * wifi_scan — OTA app entry point
 * Thin FreeRTOS wrapper around wifi_scan_main_impl.
 */

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern int wifi_scan_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);

static void ui_task(void *arg) {
    char *argv[] = {"wifi_scan"};
    (void)arg;

    wifi_scan_main_impl(1, argv);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    xTaskCreatePinnedToCore(ui_task, "wifi_scan_ui", 8192, NULL, 5, NULL, 1);
}
