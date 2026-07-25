#include "mia_emulator_runtime.h"
#include "mia_app_zip.h"
#include "mia_emulator_smsplus.h"
#include "mia_host_abi.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "smsplus.h"
#include "state.h"

#undef input

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define SMS_SURFACE_WIDTH 256u
#define SMS_SURFACE_HEIGHT 192u
#define SMS_SRAM_SIZE 0x8000u
#define SMS_INDEXED_PALETTE_SIZE 256u
#define SMS_DISPLAY_BUFFER_COUNT 2u
#define SMS_HC165_PL_PIN GPIO_NUM_2
#define SMS_HC165_CLK_PIN GPIO_NUM_39
#define SMS_HC165_DAT_PIN GPIO_NUM_38
#define COLECO_KEYPAD_COLUMNS 3u
#define COLECO_KEYPAD_ROWS 4u
#define COLECO_KEYPAD_COUNT (COLECO_KEYPAD_COLUMNS * COLECO_KEYPAD_ROWS)
#define SMSPLUS_ROM_MAX_SIZE (8u * 1024u * 1024u)

#ifndef MIA_SMSPLUS_SAVE_INTERVAL_FRAMES
#define MIA_SMSPLUS_SAVE_INTERVAL_FRAMES 60u
#endif

static uint8_t indexed_frame[SMS_SURFACE_WIDTH * SMS_SURFACE_HEIGHT];
static uint16_t palette[SMS_INDEXED_PALETTE_SIZE];
static QueueHandle_t display_ready_queue;
static QueueHandle_t display_free_queue;
static TaskHandle_t display_task_handle;
static volatile bool display_running;
static uint16_t *display_buffers[SMS_DISPLAY_BUFFER_COUNT];
static uint8_t *zip_rom_data;

static uint8_t scan_hc165_once(void) {
    gpio_set_level(SMS_HC165_PL_PIN, 0);
    esp_rom_delay_us(5);
    gpio_set_level(SMS_HC165_PL_PIN, 1);
    esp_rom_delay_us(2);
    uint8_t value = 0;
    for (unsigned bit = 0; bit < 8u; ++bit) {
        value = (uint8_t)((value << 1u) | (gpio_get_level(SMS_HC165_DAT_PIN) ? 1u : 0u));
        gpio_set_level(SMS_HC165_CLK_PIN, 1);
        esp_rom_delay_us(2);
        gpio_set_level(SMS_HC165_CLK_PIN, 0);
    }
    return value;
}

static uint32_t smsplus_host_buttons(void) {
    static const uint8_t host_key_for_hc165_bit[8] = {
        MIA_HOST_BUTTON_LEFT, MIA_HOST_BUTTON_DOWN, MIA_HOST_BUTTON_UP,
        MIA_HOST_BUTTON_RIGHT, MIA_HOST_BUTTON_Y, MIA_HOST_BUTTON_X,
        MIA_HOST_BUTTON_A, MIA_HOST_BUTTON_B,
    };
    uint32_t buttons = mia_emulator_host_buttons() & 0x3fu;
    const uint8_t first = scan_hc165_once();
    const uint8_t second = scan_hc165_once();
    const uint8_t third = scan_hc165_once();
    const uint8_t hc165 = (uint8_t)((first & second) | (first & third) | (second & third));
    for (unsigned bit = 0; bit < 8u; ++bit) {
#ifdef MIA_HC165_ACTIVE_HIGH
        const bool pressed = (hc165 & (1u << bit)) != 0u;
#else
        const bool pressed = (hc165 & (1u << bit)) == 0u;
#endif
        if (pressed) buttons |= 1u << host_key_for_hc165_bit[bit];
    }
    return buttons;
}

static void wait_smsplus_input_release(void) {
    for (unsigned guard = 0; guard < 500u; ++guard) {
        if (smsplus_host_buttons() == 0u) return;
        mia_host_delay_ms(10);
    }
}

static void draw_coleco_keypad(unsigned selected) {
    static const char *const labels[COLECO_KEYPAD_COUNT] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#",
    };
    const int32_t cell_width = 44;
    const int32_t cell_height = 36;
    const int32_t gap = 4;
    const int32_t grid_width = COLECO_KEYPAD_COLUMNS * cell_width +
                               (COLECO_KEYPAD_COLUMNS - 1u) * gap;
    const int32_t grid_x = (mia_host_screen_width() - grid_width) / 2;
    const int32_t grid_y = 42;
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_draw_text((mia_host_screen_width() - 13 * 6) / 2, 16,
                       "Coleco Keypad", MIA_HOST_WHITE, MIA_HOST_BLACK);
    for (unsigned index = 0; index < COLECO_KEYPAD_COUNT; ++index) {
        const int32_t x = grid_x + (int32_t)(index % COLECO_KEYPAD_COLUMNS) *
                                   (cell_width + gap);
        const int32_t y = grid_y + (int32_t)(index / COLECO_KEYPAD_COLUMNS) *
                                   (cell_height + gap);
        const uint8_t border = index == selected ? MIA_HOST_CYAN : MIA_HOST_GRAY;
        mia_host_fill_rect(x, y, cell_width, cell_height, border);
        mia_host_fill_rect(x + 2, y + 2, cell_width - 4, cell_height - 4,
                           MIA_HOST_BLACK);
        mia_host_draw_text(x + (cell_width - 6) / 2, y + 14, labels[index],
                           index == selected ? MIA_HOST_CYAN : MIA_HOST_WHITE,
                           MIA_HOST_BLACK);
    }
    mia_host_present();
}

static int coleco_select_key(void) {
    while (uxQueueMessagesWaiting(display_free_queue) < SMS_DISPLAY_BUFFER_COUNT) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    mia_host_audio_stop();
    wait_smsplus_input_release();
    unsigned selected = 0;
    uint32_t previous = 0;
    draw_coleco_keypad(selected);
    for (;;) {
        const uint32_t current = smsplus_host_buttons();
        const uint32_t pressed = current & ~previous;
        previous = current;
        unsigned next = selected;
        if (pressed & (1u << MIA_HOST_BUTTON_UP)) {
            next = selected >= COLECO_KEYPAD_COLUMNS
                       ? selected - COLECO_KEYPAD_COLUMNS
                       : selected + COLECO_KEYPAD_COLUMNS * (COLECO_KEYPAD_ROWS - 1u);
        } else if (pressed & (1u << MIA_HOST_BUTTON_DOWN)) {
            next = (selected + COLECO_KEYPAD_COLUMNS) % COLECO_KEYPAD_COUNT;
        } else if (pressed & (1u << MIA_HOST_BUTTON_LEFT)) {
            next = selected % COLECO_KEYPAD_COLUMNS == 0u
                       ? selected + COLECO_KEYPAD_COLUMNS - 1u
                       : selected - 1u;
        } else if (pressed & (1u << MIA_HOST_BUTTON_RIGHT)) {
            next = selected % COLECO_KEYPAD_COLUMNS == COLECO_KEYPAD_COLUMNS - 1u
                       ? selected - COLECO_KEYPAD_COLUMNS + 1u
                       : selected + 1u;
        }
        if (next != selected) {
            selected = next;
            draw_coleco_keypad(selected);
        }
        if (pressed & (1u << MIA_HOST_BUTTON_A)) {
            wait_smsplus_input_release();
            return mia_smsplus_coleco_keypad_code(selected);
        }
        if (pressed & (1u << MIA_HOST_BUTTON_B)) {
            wait_smsplus_input_release();
            return -1;
        }
        mia_host_delay_ms(10);
    }
}

static MiaSmsPlusMode target_mode(void) {
#if MIA_SMSPLUS_MODE == 0
    return MIA_SMSPLUS_MODE_SMS;
#elif MIA_SMSPLUS_MODE == 1
    return MIA_SMSPLUS_MODE_GG;
#else
    return MIA_SMSPLUS_MODE_COLECO;
#endif
}

static MiaCoreStatus file_error(const char *message) {
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_draw_text(8, 34, message, MIA_HOST_RED, MIA_HOST_BLACK);
    mia_host_draw_text(8, 54, "Fix SD files, then relaunch", MIA_HOST_WHITE, MIA_HOST_BLACK);
    mia_host_present();
    mia_host_delay_ms(2000);
    return mia_core_error(MIA_CORE_ERR_CALLBACK, message);
}

static bool has_extension(const char *path, const char *extension) {
    const char *dot = strrchr(path, '.');
    return dot != NULL && strcasecmp(dot + 1, extension) == 0;
}

static MiaCoreStatus load_save(MiaEmulatorRuntime *runtime) {
    size_t size = 0;
    MiaStorageStatus status = mia_app_save_load(&runtime->storage, &runtime->storage_target,
                                                runtime->selection.save_name, cart.sram,
                                                SMS_SRAM_SIZE, &size);
    if (status.code == MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) return mia_core_ok();
    if (status.code != MIA_STORAGE_OK || size != SMS_SRAM_SIZE) return mia_core_error(MIA_CORE_ERR_CALLBACK, "SMS Plus SRAM load failed");
    return mia_core_ok();
}

static void display_task(void *arg) {
    MiaEmulatorRuntime *runtime = arg;
    while (display_running || uxQueueMessagesWaiting(display_ready_queue) != 0u) {
        uint16_t *frame = NULL;
        if (xQueueReceive(display_ready_queue, &frame, pdMS_TO_TICKS(20)) != pdTRUE) continue;
        MiaCoreStatus status = mia_core_adapter_submit_video(
            &runtime->adapter, frame, (size_t)MIA_EMULATOR_WIDTH * MIA_EMULATOR_HEIGHT);
        if (status.code != MIA_CORE_OK) mia_host_log(status.message);
        (void)xQueueSend(display_free_queue, &frame, portMAX_DELAY);
    }
    display_task_handle = NULL;
    vTaskDelete(NULL);
}

static bool start_display_task(MiaEmulatorRuntime *runtime) {
    display_ready_queue = xQueueCreate(1, sizeof(uint16_t *));
    display_free_queue = xQueueCreate(SMS_DISPLAY_BUFFER_COUNT, sizeof(uint16_t *));
    if (display_ready_queue == NULL || display_free_queue == NULL) return false;
    const size_t frame_bytes = (size_t)MIA_EMULATOR_WIDTH * MIA_EMULATOR_HEIGHT * sizeof(uint16_t);
    for (unsigned i = 0; i < SMS_DISPLAY_BUFFER_COUNT; ++i) {
        display_buffers[i] = heap_caps_malloc(frame_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (display_buffers[i] == NULL) return false;
        (void)xQueueSend(display_free_queue, &display_buffers[i], 0);
    }
    display_running = true;
    if (xTaskCreatePinnedToCore(display_task, "sms_display", 4096, runtime, 5,
                                &display_task_handle, 1) != pdPASS) {
        display_running = false;
        return false;
    }
    return true;
}

static void stop_display_task(void) {
    display_running = false;
    for (unsigned wait = 0; display_task_handle != NULL && wait < 100u; ++wait) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (display_task_handle != NULL) {
        vTaskDelete(display_task_handle);
        display_task_handle = NULL;
    }
    if (display_ready_queue != NULL) vQueueDelete(display_ready_queue);
    if (display_free_queue != NULL) vQueueDelete(display_free_queue);
    display_ready_queue = NULL;
    display_free_queue = NULL;
    for (unsigned i = 0; i < SMS_DISPLAY_BUFFER_COUNT; ++i) {
        heap_caps_free(display_buffers[i]);
        display_buffers[i] = NULL;
    }
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    system_reset_config();
    option.sndrate = MIA_EMULATOR_SAMPLE_RATE;
    option.overscan = 0;
    option.extra_gg = 0;
    option.console = target_mode() == MIA_SMSPLUS_MODE_COLECO ? 6 :
                     target_mode() == MIA_SMSPLUS_MODE_SMS &&
                     has_extension(runtime->selection.rom_path, "sg") ? 5 : 0;
    if (has_extension(runtime->selection.rom_path, "zip")) {
        static const char *const sms_extensions[] = {"sms", "sg"};
        static const char *const gg_extensions[] = {"gg"};
        static const char *const coleco_extensions[] = {"col", "rom"};
        const char *const *extensions = target_mode() == MIA_SMSPLUS_MODE_GG ? gg_extensions :
            target_mode() == MIA_SMSPLUS_MODE_COLECO ? coleco_extensions : sms_extensions;
        const size_t extension_count = target_mode() == MIA_SMSPLUS_MODE_COLECO ||
            target_mode() == MIA_SMSPLUS_MODE_SMS ? 2u : 1u;
        char entry_name[256];
        size_t file_size = 0;
        MiaCoreStatus status = mia_app_zip_extract(runtime->selection.rom_path,
            extensions, extension_count, SMSPLUS_ROM_MAX_SIZE, &zip_rom_data,
            &file_size, entry_name, sizeof(entry_name));
        if (status.code != MIA_CORE_OK) return file_error("SMS Plus ZIP load failed");
        size_t offset = file_size > 0x4000u && ((file_size / 512u) & 1u) ? 512u : 0u;
        size_t content_size = file_size - offset;
        size_t padded_size = content_size < 0x4000u ? 0x4000u :
            (content_size + 0x3fffu) & ~(size_t)0x3fffu;
        if (padded_size != content_size) {
            uint8_t *padded = calloc(1, padded_size);
            if (padded == NULL) return file_error("SMS Plus ZIP allocation failed");
            memcpy(padded, zip_rom_data + offset, content_size);
            free(zip_rom_data);
            zip_rom_data = padded;
        } else if (offset != 0u) {
            memmove(zip_rom_data, zip_rom_data + offset, content_size);
        }
        if (target_mode() == MIA_SMSPLUS_MODE_SMS && has_extension(entry_name, "sg")) option.console = 5;
        if (!load_rom(zip_rom_data, (int)padded_size, (int)content_size))
            return file_error("SMS Plus ZIP ROM load failed");
    } else if (!load_rom_file(runtime->selection.rom_path)) {
        return file_error("SMS Plus ROM load failed");
    }
    bitmap.width = SMS_SURFACE_WIDTH;
    bitmap.height = SMS_SURFACE_HEIGHT;
    bitmap.pitch = SMS_SURFACE_WIDTH;
    bitmap.data = indexed_frame;
    system_poweron();
    return load_save(runtime);
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    (void)force;
    if (cart.sram == NULL) return mia_core_ok();
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target,
                                                 runtime->selection.save_name, reason,
                                                 cart.sram, SMS_SRAM_SIZE, NULL);
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

MiaCoreStatus mia_emulator_core_save_state(MiaEmulatorRuntime *runtime, const char *path) {
    (void)runtime;
    FILE *file = fopen(path, "wb");
    const bool saved = file != NULL && system_save_state(file) == 0 && fflush(file) == 0;
    if (file != NULL) fclose(file);
    return saved ? mia_core_ok() :
        mia_core_error(MIA_CORE_ERR_CALLBACK, "SMS Plus state save failed");
}

MiaCoreStatus mia_emulator_core_load_state(MiaEmulatorRuntime *runtime, const char *path) {
    (void)runtime;
    FILE *file = fopen(path, "rb");
    if (file == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "SMS Plus state load failed");
    system_load_state(file);
    const bool loaded = ferror(file) == 0;
    fclose(file);
    return loaded ? mia_core_ok() :
        mia_core_error(MIA_CORE_ERR_CALLBACK, "SMS Plus state load failed");
}

static MiaCoreStatus submit_frame(MiaEmulatorRuntime *runtime) {
    uint16_t *rgb_frame = NULL;
    if (xQueueReceive(display_free_queue, &rgb_frame, 0) != pdTRUE) return mia_core_ok();
    (void)render_copy_palette(palette);
    const size_t width = MIA_EMULATOR_WIDTH;
    const size_t height = MIA_EMULATOR_HEIGHT;
    const size_t x = target_mode() == MIA_SMSPLUS_MODE_GG ? (size_t)bitmap.viewport.x : 0u;
    const size_t y = target_mode() == MIA_SMSPLUS_MODE_GG ? (size_t)bitmap.viewport.y : 0u;
    for (size_t row = 0; row < height; ++row) {
        if (!mia_smsplus_convert_frame(indexed_frame + (y + row) * SMS_SURFACE_WIDTH + x,
                                       width, palette, SMS_INDEXED_PALETTE_SIZE,
                                       rgb_frame + row * width, width)) {
            (void)xQueueSend(display_free_queue, &rgb_frame, 0);
            return mia_core_error(MIA_CORE_ERR_CALLBACK, "SMS Plus palette conversion failed");
        }
    }
    if (xQueueSend(display_ready_queue, &rgb_frame, 0) != pdTRUE) {
        (void)xQueueSend(display_free_queue, &rgb_frame, 0);
    }
    return mia_core_ok();
}

static MiaCoreStatus submit_audio(MiaEmulatorRuntime *runtime) {
    const size_t count = snd.sample_count > 0 ? (size_t)snd.sample_count : 0u;
    if (count == 0u || snd.stream[0] == NULL || snd.stream[1] == NULL) return mia_core_ok();
    static int16_t stereo[2048];
    const size_t bounded = count > 1024u ? 1024u : count;
    for (size_t index = 0; index < bounded; ++index) {
        stereo[index * 2u] = (int16_t)(snd.stream[0][index] * 2.75f);
        stereo[index * 2u + 1u] = (int16_t)(snd.stream[1][index] * 2.75f);
    }
    return mia_core_adapter_submit_audio(&runtime->adapter, stereo, bounded);
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    wait_smsplus_input_release();
    if (!start_display_task(runtime)) {
        stop_display_task();
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "SMS Plus display task start failed");
    }
#if MIA_SMSPLUS_SAVE_INTERVAL_FRAMES > 0
    unsigned save_frame = 0;
#endif
    uint32_t previous_host_buttons = 0;
    int coleco_key = -1;
    unsigned coleco_key_decay = 0;
    for (;;) {
        const uint32_t host_buttons = smsplus_host_buttons();
        const uint32_t pressed_buttons = host_buttons & ~previous_host_buttons;
        previous_host_buttons = host_buttons;
        const uint32_t buttons = mia_app_input_core_mask(&runtime->hardware_target, host_buttons);
        MiaCoreStatus status = mia_core_ok();
        if (mia_app_input_menu_requested(&runtime->input, host_buttons)) {
            stop_display_task();
            return mia_core_ok();
        }
        if (target_mode() == MIA_SMSPLUS_MODE_COLECO) {
            coleco.keypad[0] = 0xff;
            coleco.keypad[1] = 0xff;
            if (coleco_key_decay > 0u) {
                coleco.keypad[0] = (uint8_t)coleco_key;
                --coleco_key_decay;
            }
            if (pressed_buttons & (1u << MIA_HOST_BUTTON_START)) {
                coleco_key = coleco_select_key();
                if (coleco_key >= 0) coleco_key_decay = 4u;
                previous_host_buttons = smsplus_host_buttons();
                continue;
            }
            if (pressed_buttons & (1u << MIA_HOST_BUTTON_SELECT)) {
                system_reset();
                continue;
            }
        }
        const MiaSmsPlusInput mapped = mia_smsplus_map_input(target_mode(), buttons);
        smsplus.input.pad[0] = mapped.pad;
        smsplus.input.system = mapped.system;
        const bool draw_frame = uxQueueMessagesWaiting(display_free_queue) != 0u;
        system_frame(draw_frame ? 0 : 1);
        if (draw_frame) status = submit_frame(runtime);
        if (status.code == MIA_CORE_OK) status = submit_audio(runtime);
#if MIA_SMSPLUS_SAVE_INTERVAL_FRAMES > 0
        if (status.code == MIA_CORE_OK && ++save_frame % MIA_SMSPLUS_SAVE_INTERVAL_FRAMES == 0u) {
            status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
        }
#endif
        if (status.code != MIA_CORE_OK) {
            stop_display_task();
            return status;
        }
    }
}
