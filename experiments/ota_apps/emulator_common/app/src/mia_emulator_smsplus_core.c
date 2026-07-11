#include "mia_emulator_runtime.h"
#include "mia_emulator_smsplus.h"
#include "mia_host_abi.h"
#include "mbedtls/sha1.h"
#include "smsplus.h"

#undef input

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SMS_SURFACE_WIDTH 256u
#define SMS_SURFACE_HEIGHT 192u
#define SMS_SRAM_SIZE 0x8000u

uint8 *mia_smsplus_coleco_bios;
static uint8_t indexed_frame[SMS_SURFACE_WIDTH * SMS_SURFACE_HEIGHT];
static uint16_t rgb_frame[SMS_SURFACE_WIDTH * SMS_SURFACE_HEIGHT];
static uint16_t palette[PALETTE_SIZE];

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

static bool has_sega_header(const uint8_t *data, size_t size, size_t *header_offset) {
    static const size_t offsets[] = {0x1ff0u, 0x3ff0u, 0x7ff0u};
    for (size_t index = 0; index < sizeof(offsets) / sizeof(offsets[0]); ++index) {
        if (size >= offsets[index] + 16u && memcmp(data + offsets[index], "TMR SEGA", 8) == 0) {
            *header_offset = offsets[index];
            return true;
        }
    }
    return false;
}

static MiaCoreStatus validate_rom(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return file_error("ROM open failed");
    uint8_t header[0x8000];
    const size_t size = fread(header, 1, sizeof(header), file);
    fclose(file);
    if (target_mode() == MIA_SMSPLUS_MODE_COLECO) {
        if (size < 2u || !((header[0] == 0x55u && header[1] == 0xaau) ||
                           (header[0] == 0xaau && header[1] == 0x55u))) {
            return file_error("Invalid Coleco ROM header");
        }
        return mia_core_ok();
    }
    size_t offset = 0;
    if (!has_sega_header(header, size, &offset)) return file_error("Invalid Sega ROM header");
    const uint8_t region = header[offset + 15u] >> 4u;
    if (target_mode() == MIA_SMSPLUS_MODE_GG && (region < 5u || region > 7u)) {
        return file_error("ROM is not Game Gear format");
    }
    return mia_core_ok();
}

typedef struct {
    FILE *file;
    uint8_t **accepted_bios;
} ColecoBiosContext;

static MiaSmsPlusBiosStatus read_coleco_bios(void *context, MiaSmsPlusBiosReadRequest *request) {
    ColecoBiosContext *bios = context;
    request->size = fread(request->buffer, 1, request->capacity, bios->file);
    return ferror(bios->file) ? MIA_SMSPLUS_BIOS_IO_FAILED : MIA_SMSPLUS_BIOS_OK;
}

static int hash_coleco_bios(void *context, MiaSmsPlusBiosHashRequest *request) {
    (void)context;
    return mbedtls_sha1(request->buffer, request->size, request->digest);
}

static void accept_coleco_bios(void *context, uint8_t *buffer) {
    ColecoBiosContext *bios = context;
    *bios->accepted_bios = buffer;
}

static MiaCoreStatus load_coleco_bios(void) {
    if (target_mode() != MIA_SMSPLUS_MODE_COLECO) return mia_core_ok();
    FILE *file = fopen("/bios/coleco.rom", "rb");
    if (file == NULL) return file_error(mia_smsplus_bios_error(MIA_SMSPLUS_BIOS_MISSING));
    uint8_t *buffer = malloc(MIA_SMSPLUS_COLECO_BIOS_SIZE + 1u);
    if (buffer == NULL) {
        fclose(file);
        return file_error("Coleco BIOS allocation failed");
    }
    ColecoBiosContext context = {file, &mia_smsplus_coleco_bios};
    const MiaSmsPlusBiosPipeline pipeline = {
        buffer, MIA_SMSPLUS_COLECO_BIOS_SIZE + 1u,
        read_coleco_bios, hash_coleco_bios, accept_coleco_bios, &context,
    };
    const MiaSmsPlusBiosStatus status = mia_smsplus_load_validate_coleco_bios(&pipeline);
    fclose(file);
    if (status != MIA_SMSPLUS_BIOS_OK) {
        free(buffer);
        return file_error(mia_smsplus_bios_error(status));
    }
    return mia_core_ok();
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

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    MiaCoreStatus status = validate_rom(runtime->selection.rom_path);
    if (status.code == MIA_CORE_OK) status = load_coleco_bios();
    if (status.code != MIA_CORE_OK) return status;
    system_reset_config();
    option.sndrate = MIA_EMULATOR_SAMPLE_RATE;
    option.overscan = 0;
    option.extra_gg = 0;
    option.console = target_mode() == MIA_SMSPLUS_MODE_COLECO ? 6 : 0;
    if (!load_rom_file(runtime->selection.rom_path)) return file_error("SMS Plus ROM load failed");
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

static MiaCoreStatus submit_frame(MiaEmulatorRuntime *runtime) {
    (void)render_copy_palette(palette);
    const size_t width = MIA_EMULATOR_WIDTH;
    const size_t height = MIA_EMULATOR_HEIGHT;
    const size_t x = target_mode() == MIA_SMSPLUS_MODE_GG ? (size_t)bitmap.viewport.x : 0u;
    const size_t y = target_mode() == MIA_SMSPLUS_MODE_GG ? (size_t)bitmap.viewport.y : 0u;
    for (size_t row = 0; row < height; ++row) {
        if (!mia_smsplus_convert_frame(indexed_frame + (y + row) * SMS_SURFACE_WIDTH + x,
                                       width, palette, PALETTE_SIZE,
                                       rgb_frame + row * width, width)) {
            return mia_core_error(MIA_CORE_ERR_CALLBACK, "SMS Plus palette conversion failed");
        }
    }
    return mia_core_adapter_submit_video(&runtime->adapter, rgb_frame, width * height);
}

static MiaCoreStatus submit_audio(MiaEmulatorRuntime *runtime) {
    const size_t count = snd.sample_count > 0 ? (size_t)snd.sample_count : 0u;
    if (count == 0u || snd.output[0] == NULL || snd.output[1] == NULL) return mia_core_ok();
    static int16_t stereo[2048];
    const size_t bounded = count > 1024u ? 1024u : count;
    for (size_t index = 0; index < bounded; ++index) {
        stereo[index * 2u] = snd.output[0][index];
        stereo[index * 2u + 1u] = snd.output[1][index];
    }
    return mia_core_adapter_submit_audio(&runtime->adapter, stereo, bounded);
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    for (unsigned frame = 1;; ++frame) {
        uint32_t buttons = 0;
        MiaCoreStatus status = mia_core_adapter_poll_input(&runtime->adapter, &buttons);
        if (status.code != MIA_CORE_OK) return status;
        if (mia_app_input_exit_requested(&runtime->input, mia_emulator_host_buttons(), mia_host_millis())) return mia_core_ok();
        const MiaSmsPlusInput mapped = mia_smsplus_map_input(target_mode(), buttons);
        smsplus.input.pad[0] = mapped.pad;
        smsplus.input.system = mapped.system;
        system_frame(0);
        status = submit_frame(runtime);
        if (status.code == MIA_CORE_OK) status = submit_audio(runtime);
        if (status.code == MIA_CORE_OK && frame % 60u == 0u) status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
        if (status.code != MIA_CORE_OK) return status;
        mia_host_delay_ms(16);
    }
}
