extern "C" {
#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"
#include <esp_rom_crc.h>
}
#include <cstring>
#include "handy.h"

#include <cstdio>
#include <new>

enum MiaLynxError { MIA_LYNX_ERR_BIOS_MISSING, MIA_LYNX_ERR_BIOS_CORRUPT, MIA_LYNX_ERR_HEADER_CORRUPT, MIA_LYNX_ERR_SAVE_CORRUPT };
static constexpr size_t LYNX_BOOT_SIZE = 512;
static constexpr uint32_t LYNX_BOOT_CRC32 = 0x0d973c9d;
static CSystem *system_instance;
static uint16_t framebuffer[160 * 160];
static SWORD audio_buffer[HANDY_AUDIO_BUFFER_LENGTH * 2];
static int rotation;

static MiaCoreStatus lynx_error(MiaLynxError code) {
    switch (code) {
        case MIA_LYNX_ERR_BIOS_MISSING: return mia_core_error(MIA_CORE_ERR_CALLBACK, "Lynx BIOS missing: /bios/lynxboot.img");
        case MIA_LYNX_ERR_BIOS_CORRUPT: return mia_core_error(MIA_CORE_ERR_CALLBACK, "Lynx BIOS corrupt: expected canonical 512-byte lynxboot.img");
        case MIA_LYNX_ERR_HEADER_CORRUPT: return mia_core_error(MIA_CORE_ERR_CALLBACK, "Lynx LNX header corrupt");
        case MIA_LYNX_ERR_SAVE_CORRUPT: return mia_core_error(MIA_CORE_ERR_CALLBACK, "Lynx EEPROM save corrupt");
    }
    return mia_core_error(MIA_CORE_ERR_CALLBACK, "Lynx unknown error");
}

static MiaCoreStatus load_bios(uint8_t (&bios)[LYNX_BOOT_SIZE]) {
    FILE *file = fopen("/bios/lynxboot.img", "rb");
    if (file == nullptr) return lynx_error(MIA_LYNX_ERR_BIOS_MISSING);
    const size_t count = fread(bios, 1, sizeof(bios), file);
    const int extra = fgetc(file);
    fclose(file);
    const uint32_t crc = esp_rom_crc32_le(UINT32_MAX, bios, sizeof(bios)) ^ UINT32_MAX;
    return count == sizeof(bios) && extra == EOF && crc == LYNX_BOOT_CRC32 ? mia_core_ok() : lynx_error(MIA_LYNX_ERR_BIOS_CORRUPT);
}

static MiaCoreStatus validate_header(const char *path) {
    uint8_t header[64];
    FILE *file = fopen(path, "rb");
    if (file == nullptr) return lynx_error(MIA_LYNX_ERR_HEADER_CORRUPT);
    const size_t count = fread(header, 1, sizeof(header), file);
    fclose(file);
    const bool magic = count == sizeof(header) && memcmp(header, "LYNX", 4) == 0;
    const uint16_t page0 = static_cast<uint16_t>(header[4] | header[5] << 8);
    const uint16_t page1 = static_cast<uint16_t>(header[6] | header[7] << 8);
    const uint16_t version = static_cast<uint16_t>(header[8] | header[9] << 8);
    return magic && page0 > 0 && page1 > 0 && version == 1 ? mia_core_ok() : lynx_error(MIA_LYNX_ERR_HEADER_CORRUPT);
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
    MiaCoreStatus status = validate_header(runtime->selection.rom_path);
    uint8_t bios[LYNX_BOOT_SIZE];
    if (status.code == MIA_CORE_OK) status = load_bios(bios);
    if (status.code != MIA_CORE_OK) return status;
    system_instance = new (std::nothrow) CSystem(runtime->selection.rom_path, MIKIE_PIXEL_FORMAT_16BPP_565, 32000);
    if (system_instance == nullptr || system_instance->mFileType != HANDY_FILETYPE_LNX) return lynx_error(MIA_LYNX_ERR_HEADER_CORRUPT);
    memcpy(system_instance->mBiosRom, bios, sizeof(bios));
    system_instance->Reset();
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
