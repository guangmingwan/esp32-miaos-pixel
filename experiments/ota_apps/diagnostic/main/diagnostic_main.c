/*
 * diagnostic_main — ported from experiments/elf_apps/diagnostic/src/main.c
 *
 * Uses the mia_host_* ABI (declared in mia_host_abi.h) for all display and
 * input operations.  Renamed from main() so that app_main.c can call it from
 * a FreeRTOS task.
 */

#include "diagnostic_i18n.h"
#include "mia_host_abi.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <esp_heap_caps.h>
#include <esp_ota_ops.h>

static const char *BUTTON_LABELS[14] = {
    "BOOT", "ST", "M", "L", "R", "SEL", "A",
    "B", "X", "Y", "UP", "DN", "LEFT", "RIGHT",
};

static uint16_t seen_buttons;

static void draw_status_box(int32_t x, int32_t y, const char *label, uint8_t ok) {
    const int32_t width = 70;
    uint8_t color = ok ? MIA_HOST_GREEN : MIA_HOST_RED;
    int32_t text_x = x + (width - mia_host_text_width(label)) / 2;
    if (text_x < x + 2) text_x = x + 2;
    mia_host_fill_rect(x, y, width, 18, MIA_HOST_BLACK);
    mia_host_fill_rect(x, y, width, 1, color);
    mia_host_fill_rect(x, y + 17, width, 1, color);
    mia_host_fill_rect(x, y, 1, 18, color);
    mia_host_fill_rect(x + width - 1, y, 1, 18, color);
    mia_host_draw_text(text_x, mia_host_text_y_centered(y, 18), label, color,
                       MIA_HOST_BLACK);
}

static void draw_button_grid(const DiagnosticText *text) {
    mia_host_fill_rect(0, 112, mia_host_screen_width(), 64, MIA_HOST_BLACK);
    mia_host_draw_text(2, 112, text->btn, MIA_HOST_WHITE, MIA_HOST_BLACK);
    for (uint8_t index = 0; index < 14; ++index) {
        int32_t col = index % 7;
        int32_t row = index / 7;
        int32_t x = 4 + col * 45;
        int32_t y = 126 + row * 24;
        uint8_t down = mia_host_button_down(index);
        uint8_t seen = (seen_buttons & (1u << index)) != 0;
        uint8_t color = down ? MIA_HOST_YELLOW : (seen ? MIA_HOST_GREEN : MIA_HOST_BLUE);
        int32_t text_x = x + (40 - mia_host_text_width(BUTTON_LABELS[index])) / 2;
        if (text_x < x + 1) text_x = x + 1;
        mia_host_fill_rect(x, y, 40, 18, color);
        mia_host_draw_text(text_x, mia_host_text_y_centered(y, 18),
                           BUTTON_LABELS[index], MIA_HOST_BLACK, color);
    }
}

static uint16_t button_mask(void) {
    uint16_t mask = 0;
    for (uint8_t index = 0; index < 14; ++index) {
        if (mia_host_button_down(index)) mask |= 1u << index;
    }
    return mask;
}

static uint8_t ota_slot_ok(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL || running->type != ESP_PARTITION_TYPE_APP ||
        (running->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_0 &&
         running->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_1)) {
        return 0;
    }
    esp_ota_img_states_t state;
    return esp_ota_get_state_partition(running, &state) == ESP_OK &&
           state != ESP_OTA_IMG_INVALID && state != ESP_OTA_IMG_ABORTED;
}

static void draw_diagnostic(uint32_t frame) {
    const DiagnosticText *text = diagnostic_text();
    MiaHostSystemInfo system_info;
    MiaHostBatteryInfo battery_info;
    MiaHostDateTime rtc_info;
    char line[48];

    memset(&system_info, 0, sizeof(system_info));
    memset(&battery_info, 0, sizeof(battery_info));
    uint8_t system_ok = mia_host_get_system_info(&system_info);
    uint8_t battery_ok = mia_host_read_battery(&battery_info);
    uint8_t rtc_ok = mia_host_rtc_read(&rtc_info);
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
    mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->title, MIA_HOST_BLACK,
                       MIA_HOST_YELLOW);

    snprintf(line, sizeof(line), "%s r%u %uMHz Heap %luK",
             system_ok ? system_info.chip_model : "?", system_info.chip_revision,
             system_info.cpu_mhz,
             (unsigned long)system_info.free_heap_kb);
    mia_host_draw_text(4, 34, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), "Flash %luMB PSRAM %luMB SD %s",
             (unsigned long)system_info.flash_mb,
             (unsigned long)(heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / (1024 * 1024)),
             system_info.sd_ready ? "YES" : "NO");
    mia_host_draw_text(4, 48, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), battery_ok ? "VBAT %lu.%03luV (%ld)" : "VBAT unavailable",
             (unsigned long)(battery_info.millivolts / 1000),
             (unsigned long)(battery_info.millivolts % 1000), (long)battery_info.raw);
    mia_host_draw_text(4, 62, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    draw_status_box(4, 82, text->tft_ok, system_ok && system_info.tft_ready);
    draw_status_box(82, 82, text->sd_ok, system_ok && system_info.sd_ready);
    draw_status_box(160, 82, text->ota_ok, ota_slot_ok());
    draw_status_box(238, 82, text->rtc_ok, rtc_ok);
    draw_button_grid(text);

    snprintf(line, sizeof(line), "Uptime %lus F%lu",
             (unsigned long)(mia_host_millis() / 1000), (unsigned long)frame);
    mia_host_draw_text(4, 196, line, MIA_HOST_CYAN, MIA_HOST_BLACK);
    mia_host_draw_text(4, 210, text->exit_hint, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_draw_text(4, 224, text->boot_note, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
}

int diagnostic_main_impl(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    uint32_t last_render_ms = 0;
    uint32_t frame = 0;
    uint16_t last_button_mask = 0;
    draw_diagnostic(frame);
    while (1) {
        mia_host_buttons_poll();
        if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
            mia_host_button_down(MIA_HOST_BUTTON_START)) {
            break;
        }
        uint16_t current_button_mask = button_mask();
        seen_buttons |= current_button_mask;
        uint32_t now_ms = mia_host_millis();
        if (current_button_mask != last_button_mask || now_ms - last_render_ms >= 500) {
            last_render_ms = now_ms;
            last_button_mask = current_button_mask;
            draw_diagnostic(frame++);
        }
        mia_host_delay_ms(20);
    }

    mia_host_clear(MIA_HOST_BLACK);
    mia_host_present();
    return 0;
}
