/*
 * diagnostic_main — ported from experiments/elf_apps/diagnostic/src/main.c
 *
 * Uses the mia_host_* ABI (declared in mia_host_abi.h) for all display and
 * input operations.  Renamed from main() so that app_main.c can call it from
 * a FreeRTOS task.
 */

#include "mia_host_abi.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

static const char *BUTTON_LABELS[14] = {
    "BOOT", "ST", "M", "L", "R", "SEL", "A",
    "B", "X", "Y", "UP", "DN", "LF", "RT",
};

static void draw_status_box(int32_t x, int32_t y, const char *label, uint8_t ok) {
    uint8_t color = ok ? MIA_HOST_GREEN : MIA_HOST_RED;
    mia_host_fill_rect(x, y, 68, 18, MIA_HOST_BLACK);
    mia_host_fill_rect(x, y, 68, 1, color);
    mia_host_fill_rect(x, y + 17, 68, 1, color);
    mia_host_fill_rect(x, y, 1, 18, color);
    mia_host_fill_rect(x + 67, y, 1, 18, color);
    mia_host_draw_text(x + 8, y + 5, label, color, MIA_HOST_BLACK);
}

static void draw_button_grid(void) {
    mia_host_fill_rect(0, 112, mia_host_screen_width(), 64, MIA_HOST_BLACK);
    mia_host_draw_text(2, 112, "BTN", MIA_HOST_WHITE, MIA_HOST_BLACK);
    for (uint8_t index = 0; index < 14; ++index) {
        int32_t col = index % 7;
        int32_t row = index / 7;
        int32_t x = 4 + col * 45;
        int32_t y = 126 + row * 24;
        uint8_t down = mia_host_button_down(index);
        uint8_t color = down ? MIA_HOST_YELLOW : MIA_HOST_BLUE;
        mia_host_fill_rect(x, y, 40, 18, color);
        mia_host_draw_text(x + 5, y + 5, BUTTON_LABELS[index], MIA_HOST_BLACK, color);
    }
}

static void draw_diagnostic(uint32_t frame) {
    MiaHostSystemInfo system_info;
    MiaHostBatteryInfo battery_info;
    char line[48];

    mia_host_get_system_info(&system_info);
    mia_host_read_battery(&battery_info);
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
    mia_host_draw_text(4, 6, "ESP32-S3 Diagnostic", MIA_HOST_BLACK, MIA_HOST_YELLOW);

    snprintf(line, sizeof(line), "%s r%u %uMHz Heap %luK", system_info.chip_model,
             system_info.chip_revision, system_info.cpu_mhz,
             (unsigned long)system_info.free_heap_kb);
    mia_host_draw_text(4, 34, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), "Flash %luMB SD ready %s",
             (unsigned long)system_info.flash_mb, system_info.sd_ready ? "YES" : "NO");
    mia_host_draw_text(4, 48, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    snprintf(line, sizeof(line), "VBAT %lu.%03luV (%ld)",
             (unsigned long)(battery_info.millivolts / 1000),
             (unsigned long)(battery_info.millivolts % 1000), (long)battery_info.raw);
    mia_host_draw_text(4, 62, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

    draw_status_box(8, 82, "TFT OK", system_info.tft_ready);
    draw_status_box(84, 82, "SD OK", system_info.sd_ready);
    draw_status_box(160, 82, "OTA OK", 1);
    draw_button_grid();

    snprintf(line, sizeof(line), "Uptime %lus F%lu",
             (unsigned long)(mia_host_millis() / 1000), (unsigned long)frame);
    mia_host_draw_text(4, 196, line, MIA_HOST_CYAN, MIA_HOST_BLACK);
    mia_host_draw_text(4, 210, "Hold SEL+ST to exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_draw_text(4, 224, "BOOT and ST mirror GPIO0", MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
}

int diagnostic_main_impl(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    uint32_t last_render_ms = 0;
    uint32_t frame = 0;
    draw_diagnostic(frame);
    while (1) {
        mia_host_buttons_poll();
        if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
            mia_host_button_down(MIA_HOST_BUTTON_START)) {
            break;
        }
        uint32_t now_ms = mia_host_millis();
        if (now_ms - last_render_ms >= 500) {
            last_render_ms = now_ms;
            draw_diagnostic(frame++);
        }
        mia_host_delay_ms(20);
    }

    mia_host_clear(MIA_HOST_BLACK);
    mia_host_present();
    return 0;
}
