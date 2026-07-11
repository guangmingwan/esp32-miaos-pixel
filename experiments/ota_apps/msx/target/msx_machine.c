#include "msx_machine.h"
#include "msx_policy.h"
#include "mia_host_abi.h"

#include <stdlib.h>
#include <string.h>

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
static int joy_state;
static int last_key;
static bool exit_requested;
static bool key_held;
static MiaMsxKeyboard keyboard;
static char save_path[192];

const char *Title = "fMSX 6.0";
const char *Disks[2][MAXDISKS + 1];

void mia_msx_set_save_name(const char *name) {
    snprintf(save_path, sizeof(save_path), "/saves/msx/%s.sta", name);
}

static uint32_t buttons(void) {
    mia_host_buttons_poll();
    uint32_t bits = 0;
    for (uint8_t key = MIA_HOST_BUTTON_BOOT; key <= MIA_HOST_BUTTON_RIGHT; ++key) {
        if (mia_host_button_down(key)) bits |= 1u << key;
    }
    return bits;
}

static bool down(uint32_t bits, uint8_t key) { return (bits & (1u << key)) != 0; }

static void update_input(void) {
    static uint32_t previous;
    const uint32_t current = buttons();
    const uint32_t pressed = current & ~previous;
    if (down(current, MIA_HOST_BUTTON_SELECT) && down(current, MIA_HOST_BUTTON_START)) {
        exit_requested = true;
    } else if (down(pressed, MIA_HOST_BUTTON_SELECT)) {
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
    if (pixels == NULL || !mia_host_audio_open(AUDIO_RATE, 2, 16)) return 0;
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
}

void SetColor(byte index, byte red, byte green, byte blue) {
    uint16_t color = (uint16_t)(((red & 0xf8) << 8) | ((green & 0xfc) << 3) | (blue >> 3));
    color = (uint16_t)((color >> 8) | (color << 8));
    if (index == 0) XPal0 = color; else XPal[index] = color;
}

void PutImage(void) {
    if (keyboard.visible) DrawKeyboard(&screen, mia_msx_keyboard_key(&keyboard));
    mia_host_present_rgb565(pixels, WIDTH, HEIGHT, WIDTH * 2);
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
