#include "msx_machine.h"
#include "msx_policy.h"
#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define WIDTH 256
#define HEIGHT 228
#define AUDIO_RATE 32000

void PutImage(void);
uint16_t BPal[256];
uint16_t XPal[80];
uint16_t XPal0;
uint16_t *XBuf;

#include <fmsx.h>

static Image screen;
static uint16_t *pixels;
static uint16_t *screen_pixels;
static int joy_state;
static int last_key;
static bool exit_requested;
static bool key_held;
static bool menu_button_down;
static bool return_to_picker;
static MiaMsxKeyboard keyboard;
static char save_path[192];
static char state_path[192];
static char state_temporary[256];
static char state_backup[256];
static char state_name[160];
static uint8_t state_slot;
static MiaDisplayScaleMode scale_mode = MIA_DISPLAY_SCALE_FIT;

const char *Title = "fMSX 6.0";
const char *Disks[2][MAXDISKS + 1];

void mia_msx_set_save_name(const char *name) {
    const char *dot = strrchr(name, '.');
    const size_t stem_length = dot == NULL ? strlen(name) : (size_t)(dot - name);
    snprintf(save_path, sizeof(save_path), "/saves/msx/%s.sta", name);
    snprintf(state_name, sizeof(state_name), "%.*s", (int)stem_length, name);
    snprintf(state_path, sizeof(state_path), "/sd/saves/msx/%s.state%u",
             state_name, state_slot);
    snprintf(state_temporary, sizeof(state_temporary), "%s.tmp", state_path);
    snprintf(state_backup, sizeof(state_backup), "%s.bak", state_path);
    (void)mkdir("/sd/saves", 0775);
    (void)mkdir("/sd/saves/msx", 0775);
}

bool mia_msx_return_to_picker_requested(void) { return return_to_picker; }

static uint32_t buttons(void) {
    mia_host_buttons_poll();
    uint32_t bits = 0;
    for (uint8_t key = MIA_HOST_BUTTON_BOOT; key <= MIA_HOST_BUTTON_RIGHT; ++key) {
        if (mia_host_button_down(key)) bits |= 1u << key;
    }
    return bits;
}

static bool down(uint32_t bits, uint8_t key) { return (bits & (1u << key)) != 0; }

static void select_state_slot(uint8_t slot) {
    state_slot = slot % 10u;
    snprintf(state_path, sizeof(state_path), "/sd/saves/msx/%s.state%u",
             state_name, state_slot);
    snprintf(state_temporary, sizeof(state_temporary), "%s.tmp", state_path);
    snprintf(state_backup, sizeof(state_backup), "%s.bak", state_path);
}

static bool save_state_slot(void) {
    (void)remove(state_temporary);
    if (!SaveSTA(state_temporary)) {
        (void)remove(state_temporary);
        return false;
    }
    (void)remove(state_backup);
    const bool had_previous = rename(state_path, state_backup) == 0;
    if (!had_previous && errno != ENOENT) {
        (void)remove(state_temporary);
        return false;
    }
    if (rename(state_temporary, state_path) != 0) {
        if (had_previous) (void)rename(state_backup, state_path);
        (void)remove(state_temporary);
        return false;
    }
    if (had_previous) (void)remove(state_backup);
    return true;
}

static void update_input(void) {
    static uint32_t previous;
    const uint32_t current = buttons();
    const uint32_t pressed = current & ~previous;
    const bool menu_down = down(current, MIA_HOST_BUTTON_M);
    if (menu_down && !menu_button_down) {
        menu_button_down = true;
        mia_host_audio_stop();
        MiaEmulatorMenuNotice notice = MIA_EMULATOR_MENU_NOTICE_NONE;
        for (;;) {
            const MiaEmulatorMenuAction action = mia_emulator_menu_run(
                "msx", screen_pixels, notice, &state_slot, &scale_mode);
            select_state_slot(state_slot);
            notice = MIA_EMULATOR_MENU_NOTICE_NONE;
            if (action == MIA_EMULATOR_MENU_SAVE_STATE) {
                notice = save_state_slot() ? MIA_EMULATOR_MENU_NOTICE_STATE_SAVED :
                                             MIA_EMULATOR_MENU_NOTICE_STATE_ERROR;
                continue;
            }
            if (action == MIA_EMULATOR_MENU_LOAD_STATE) {
                if (access(state_path, F_OK) != 0) {
                    notice = MIA_EMULATOR_MENU_NOTICE_STATE_MISSING;
                    continue;
                }
                if (!LoadSTA(state_path)) {
                    notice = MIA_EMULATOR_MENU_NOTICE_STATE_ERROR;
                    continue;
                }
                mia_host_fill_screen_rgb565(0);
                break;
            }
            return_to_picker = action == MIA_EMULATOR_MENU_ROM_PICKER;
            exit_requested = action != MIA_EMULATOR_MENU_RESUME;
            if (action == MIA_EMULATOR_MENU_RESUME) mia_host_fill_screen_rgb565(0);
            break;
        }
    } else if (!menu_down) {
        menu_button_down = false;
    }
    if (menu_down) {
        previous = current;
        return;
    }
    if (down(pressed, MIA_HOST_BUTTON_SELECT)) {
        keyboard.visible = !keyboard.visible;
    } else if (keyboard.visible) {
        if (down(pressed, MIA_HOST_BUTTON_LEFT)) mia_msx_keyboard_move(&keyboard, -1, 0);
        if (down(pressed, MIA_HOST_BUTTON_RIGHT)) mia_msx_keyboard_move(&keyboard, 1, 0);
        if (down(pressed, MIA_HOST_BUTTON_UP)) mia_msx_keyboard_move(&keyboard, 0, -1);
        if (down(pressed, MIA_HOST_BUTTON_DOWN)) mia_msx_keyboard_move(&keyboard, 0, 1);
        if (down(pressed, MIA_HOST_BUTTON_B)) keyboard.visible = false;
        const uint8_t key = mia_msx_keyboard_key(&keyboard);
        if (key != 0 && down(current, MIA_HOST_BUTTON_A)) {
            KBD_SET(key);
            key_held = true;
        } else if (key_held) {
            KBD_RES(key);
            key_held = false;
        }
    } else {
        joy_state = 0;
        if (down(current, MIA_HOST_BUTTON_LEFT)) joy_state |= JST_LEFT;
        if (down(current, MIA_HOST_BUTTON_RIGHT)) joy_state |= JST_RIGHT;
        if (down(current, MIA_HOST_BUTTON_UP)) joy_state |= JST_UP;
        if (down(current, MIA_HOST_BUTTON_DOWN)) joy_state |= JST_DOWN;
        if (down(current, MIA_HOST_BUTTON_A)) joy_state |= JST_FIREA;
        if (down(current, MIA_HOST_BUTTON_B)) joy_state |= JST_FIREB;
    }
    previous = current;
}

int InitMachine(void) {
    pixels = malloc(WIDTH * HEIGHT * sizeof(*pixels));
    screen_pixels = calloc(MIA_DISPLAY_PIXELS, sizeof(*screen_pixels));
    if (pixels == NULL || screen_pixels == NULL || !mia_host_audio_open(AUDIO_RATE, 2, 16)) {
        free(pixels);
        free(screen_pixels);
        pixels = NULL;
        screen_pixels = NULL;
        return 0;
    }
    screen = (Image){.Data = pixels, .W = WIDTH, .H = HEIGHT, .L = WIDTH, .D = 16};
    XBuf = pixels;
    SetScreenDepth(16);
    SetVideo(&screen, 0, 0, WIDTH, HEIGHT);
    InitSound(AUDIO_RATE, 150);
    SetChannels(64, 0xffffffff);
    return 1;
}

void TrashMachine(void) {
    if (save_path[0] != '\0') SaveSTA(save_path);
    TrashSound();
    mia_host_audio_close();
    free(pixels);
    free(screen_pixels);
    pixels = NULL;
    screen_pixels = NULL;
}

void SetColor(byte index, byte red, byte green, byte blue) {
    uint16_t color = (uint16_t)(((red & 0xf8) << 8) | ((green & 0xfc) << 3) | (blue >> 3));
    color = (uint16_t)((color >> 8) | (color << 8));
    if (index == 0) XPal0 = color; else XPal[index] = color;
}

void PutImage(void) {
    if (keyboard.visible) DrawKeyboard(&screen, mia_msx_keyboard_key(&keyboard));
    for (uint32_t y = 0; y < MIA_DISPLAY_HEIGHT; ++y) {
        const uint32_t source_y = y * HEIGHT / MIA_DISPLAY_HEIGHT;
        for (uint32_t x = 0; x < MIA_DISPLAY_WIDTH; ++x) {
            const uint32_t source_x = x * WIDTH / MIA_DISPLAY_WIDTH;
            screen_pixels[y * MIA_DISPLAY_WIDTH + x] =
                __builtin_bswap16(pixels[source_y * WIDTH + source_x]);
        }
    }
    mia_host_present_rgb565(screen_pixels, MIA_DISPLAY_WIDTH, MIA_DISPLAY_HEIGHT,
                            MIA_DISPLAY_WIDTH * sizeof(uint16_t));
}

unsigned int Joystick(void) { update_input(); return (unsigned)joy_state; }
void Keyboard(void) { update_input(); if (exit_requested) ExitNow = 1; }
unsigned int Mouse(byte number) { (void)number; return 0; }
int ShowVideo(void) { PutImage(); return 1; }
unsigned int GetJoystick(void) { update_input(); return 0; }
unsigned int GetMouse(void) { return 0; }
unsigned int GetKey(void) { update_input(); unsigned key = (unsigned)last_key; last_key = 0; return key; }
unsigned int WaitKey(void) { while (!GetKey() && !exit_requested) mia_host_delay_ms(10); return GetKey(); }
unsigned int WaitKeyOrMouse(void) { return WaitKey(); }
unsigned int InitAudio(unsigned int rate, unsigned int latency) { (void)rate; (void)latency; return AUDIO_RATE; }
void TrashAudio(void) {}
unsigned int GetFreeAudio(void) { return 1024; }
void PlayAllSound(int usec) { RenderAndPlayAudio((unsigned)(2LL * usec * AUDIO_RATE / 1000000)); }
unsigned int WriteAudio(sample *data, unsigned int length) {
    const unsigned frames = length / 2;
    return mia_host_audio_write_pcm16(data, frames, 2) < 0 ? 0 : length;
}
