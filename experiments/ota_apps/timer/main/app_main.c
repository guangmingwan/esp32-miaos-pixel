#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern int timer_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);

static void ui_task(void *arg) {
    char *argv[] = {"timer"};
    (void)arg;

    timer_main_impl(1, argv);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    xTaskCreatePinnedToCore(ui_task, "timer_ui", 32768, NULL, 5, NULL, 1);
}
