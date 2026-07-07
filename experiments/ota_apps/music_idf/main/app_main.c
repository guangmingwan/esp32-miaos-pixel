#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern int music_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);

static void ui_task(void *arg) {
    char *argv[] = {"music"};
    (void)arg;

    music_main_impl(1, argv);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    xTaskCreatePinnedToCore(ui_task, "music_ui", 65536, NULL, 5, NULL, 1);
}
