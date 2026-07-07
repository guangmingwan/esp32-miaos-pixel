/*
 * hello — OTA app entry point
 * Thin FreeRTOS wrapper around hello_main_impl.
 */

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern int hello_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);

static void ui_task(void *arg) {
    char *argv[] = {"hello"};
    (void)arg;

    hello_main_impl(1, argv);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    xTaskCreatePinnedToCore(ui_task, "hello_ui", 8192, NULL, 5, NULL, 1);
}
