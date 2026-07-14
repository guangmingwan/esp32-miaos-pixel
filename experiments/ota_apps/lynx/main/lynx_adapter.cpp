extern "C" {
#include "mia_emulator_runtime.h"
#include "mia_app_zip.h"
#include "mia_host_abi.h"
}
#include <cstring>
#include "handy.h"

#include <cstdio>
#include <new>
#include <strings.h>

enum MiaLynxError { MIA_LYNX_ERR_HEADER_CORRUPT, MIA_LYNX_ERR_SAVE_CORRUPT };
static CSystem *system_instance;
static uint16_t framebuffer[160 * 160];
static SWORD audio_buffer[HANDY_AUDIO_BUFFER_LENGTH * 2];
static int rotation;

static MiaCoreStatus lynx_error(MiaLynxError code) {
    switch (code) {
        case MIA_LYNX_ERR_HEADER_CORRUPT: return mia_core_error(MIA_CORE_ERR_CALLBACK, "Lynx LNX header corrupt");
        case MIA_LYNX_ERR_SAVE_CORRUPT: return mia_core_error(MIA_CORE_ERR_CALLBACK, "Lynx EEPROM save corrupt");
    }
    return mia_core_error(MIA_CORE_ERR_CALLBACK, "Lynx unknown error");
}

static void configure_rotation() {
    rotation = system_instance->mCart->CartGetRotate();
    if (rotation == CART_ROTATE_LEFT) system_instance->mMikie->SetRotation(MIKIE_ROTATE_L);
    else if (rotation == CART_ROTATE_RIGHT) system_instance->mMikie->SetRotation(MIKIE_ROTATE_R);
    else system_instance->mMikie->SetRotation(MIKIE_NO_ROTATE);
}

static ULONG map_input(uint32_t input) {
    ULONG result = 0;
    const ULONG up = rotation == CART_ROTATE_LEFT ? BUTTON_RIGHT : rotation == CART_ROTATE_RIGHT ? BUTTON_LEFT : BUTTON_UP;
    const ULONG down = rotation == CART_ROTATE_LEFT ? BUTTON_LEFT : rotation == CART_ROTATE_RIGHT ? BUTTON_RIGHT : BUTTON_DOWN;
    const ULONG left = rotation == CART_ROTATE_LEFT ? BUTTON_UP : rotation == CART_ROTATE_RIGHT ? BUTTON_DOWN : BUTTON_LEFT;
    const ULONG right = rotation == CART_ROTATE_LEFT ? BUTTON_DOWN : rotation == CART_ROTATE_RIGHT ? BUTTON_UP : BUTTON_RIGHT;
    if (input & MIA_APP_CORE_INPUT_UP) result |= up;
    if (input & MIA_APP_CORE_INPUT_DOWN) result |= down;
    if (input & MIA_APP_CORE_INPUT_LEFT) result |= left;
    if (input & MIA_APP_CORE_INPUT_RIGHT) result |= right;
    if (input & MIA_APP_CORE_INPUT_A) result |= BUTTON_A;
    if (input & MIA_APP_CORE_INPUT_B) result |= BUTTON_B;
    if (input & MIA_APP_CORE_INPUT_START) result |= BUTTON_OPT2;
    if (input & MIA_APP_CORE_INPUT_SELECT) result |= BUTTON_OPT1;
    return result;
}

extern "C" MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    const char *dot = std::strrchr(runtime->selection.rom_path, '.');
    const bool zipped = dot != nullptr && strcasecmp(dot + 1, "zip") == 0;
    if (zipped) {
        static const char *const extensions[] = {"lnx"};
        uint8_t *data = nullptr;
        size_t size = 0;
        MiaCoreStatus status = mia_app_zip_extract(runtime->selection.rom_path,
            extensions, 1u, 8u * 1024u * 1024u, &data, &size, nullptr, 0u);
        if (status.code != MIA_CORE_OK) return status;
        system_instance = new (std::nothrow) CSystem(data, static_cast<ULONG>(size), MIKIE_PIXEL_FORMAT_16BPP_565, 32000);
        free(data);
    } else {
        system_instance = new (std::nothrow) CSystem(runtime->selection.rom_path, MIKIE_PIXEL_FORMAT_16BPP_565, 32000);
    }
    if (system_instance == nullptr || system_instance->mFileType == HANDY_FILETYPE_ILLEGAL) return lynx_error(MIA_LYNX_ERR_HEADER_CORRUPT);
    configure_rotation();
    gPrimaryFrameBuffer = reinterpret_cast<UBYTE *>(framebuffer);
    gAudioBuffer = audio_buffer;
    gAudioEnabled = 1;
    uint8_t save[2048]; size_t save_size = 0;
    MiaStorageStatus loaded = mia_app_save_load(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, save, sizeof(save), &save_size);
    if (loaded.code == MIA_STORAGE_OK) {
        if (save_size != static_cast<size_t>(system_instance->mEEPROM->Size())) return lynx_error(MIA_LYNX_ERR_SAVE_CORRUPT);
        system_instance->mEEPROM->InitFrom(reinterpret_cast<char *>(save), static_cast<int>(save_size));
    } else if (loaded.code != MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) return mia_core_error(MIA_CORE_ERR_CALLBACK, loaded.message);
    return mia_core_ok();
}

extern "C" MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    (void)force;
    uint8_t save[2048];
    const int size = system_instance->mEEPROM->ExportTo(reinterpret_cast<char *>(save), sizeof(save));
    if (size <= 0) return mia_core_ok();
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, reason, save, static_cast<size_t>(size), nullptr);
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

extern "C" MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    for (;;) {
        uint32_t input = 0;
        MiaCoreStatus status = mia_core_adapter_poll_input(&runtime->adapter, &input);
        if (status.code != MIA_CORE_OK) return status;
        if (mia_app_input_exit_requested(&runtime->input, mia_emulator_host_buttons(), mia_host_millis())) return mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CLEAN_EXIT, true);
        system_instance->SetButtonData(map_input(input));
        system_instance->UpdateFrame(true);
        status = mia_core_adapter_submit_video(&runtime->adapter, framebuffer, 160u * 102u);
        if (status.code != MIA_CORE_OK) return status;
        if (gAudioBufferPointer > 0) status = mia_core_adapter_submit_audio(&runtime->adapter, audio_buffer, gAudioBufferPointer / 2u);
        gAudioBufferPointer = 0;
        if (status.code != MIA_CORE_OK) return status;
    }
}
