#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"

#include <stdlib.h>

#ifndef MIA_EMULATOR_TARGET
#error "MIA_EMULATOR_TARGET is required"
#endif

#ifndef MIA_EMULATOR_SCALE_MODE
#define MIA_EMULATOR_SCALE_MODE MIA_DISPLAY_SCALE_FIT
#endif

static const char *const extensions[] = {
#ifdef MIA_EMULATOR_GNUBOY
    MIA_EMULATOR_EXTENSION,
#endif
#ifdef MIA_EMULATOR_GW
    "gw",
#endif
#ifdef MIA_EMULATOR_SMSPLUS
    MIA_EMULATOR_EXTENSION,
#ifdef MIA_EMULATOR_SECOND_EXTENSION
    MIA_EMULATOR_SECOND_EXTENSION,
#endif
#ifdef MIA_EMULATOR_THIRD_EXTENSION
    MIA_EMULATOR_THIRD_EXTENSION,
#endif
#ifdef MIA_EMULATOR_FOURTH_EXTENSION
    MIA_EMULATOR_FOURTH_EXTENSION,
#endif
#endif
#ifdef MIA_EMULATOR_GBA
    "gba",
#endif
};

#ifdef MIA_EMULATOR_SECOND_ROM_ROOT
static const char *const alternate_rom_roots[] = {
    MIA_EMULATOR_SECOND_ROM_ROOT,
#ifdef MIA_EMULATOR_THIRD_ROM_ROOT
    MIA_EMULATOR_THIRD_ROM_ROOT,
#endif
};
#define MIA_ALTERNATE_ROM_ROOTS alternate_rom_roots
#define MIA_ALTERNATE_ROM_ROOT_COUNT (sizeof(alternate_rom_roots) / sizeof(alternate_rom_roots[0]))
#else
#define MIA_ALTERNATE_ROM_ROOTS NULL
#define MIA_ALTERNATE_ROM_ROOT_COUNT 0u
#endif

MiaEmulatorRuntime mia_emulator_runtime;

static bool allocate_runtime(MiaEmulatorRuntime *runtime) {
    runtime->display = malloc(MIA_DISPLAY_PIXELS * sizeof(uint16_t));
    runtime->audio_queue = malloc(4096u * sizeof(int16_t));
    runtime->audio_drain = malloc(1024u * sizeof(int16_t));
    return runtime->display != NULL && runtime->audio_queue != NULL && runtime->audio_drain != NULL;
}

int mia_emulator_main_impl(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    MiaEmulatorRuntime *runtime = &mia_emulator_runtime;
    runtime->storage = (MiaStorageContext){"/sd"};
    runtime->storage_target = (MiaStorageTarget){
        MIA_EMULATOR_TARGET, MIA_EMULATOR_ROM_ROOT, MIA_EMULATOR_SAVE_ROOT,
        extensions, sizeof(extensions) / sizeof(extensions[0]), NULL, 0,
        MIA_ALTERNATE_ROM_ROOTS, MIA_ALTERNATE_ROM_ROOT_COUNT};
    runtime->hardware_target = (MiaHardwareTarget){MIA_EMULATOR_TARGET, {MIA_EMULATOR_WIDTH, MIA_EMULATOR_HEIGHT}, MIA_EMULATOR_SAMPLE_RATE, NULL, 0};
    if (!allocate_runtime(runtime)) {
        mia_host_log("emulator buffers allocation failed");
        return 1;
    }
    runtime->video = (MiaAppVideoSink){runtime->display, MIA_DISPLAY_PIXELS, MIA_EMULATOR_SCALE_MODE};
    mia_app_input_init(&runtime->input, 250);
    (void)mia_app_audio_init(&runtime->audio, runtime->audio_queue, 2048);
    MiaStorageStatus pick = mia_emulator_picker_run(&runtime->storage, &runtime->storage_target, &runtime->selection);
    if (pick.code != MIA_STORAGE_OK) return 1;
    const MiaRuntimeTarget target = {MIA_EMULATOR_TARGET, "Emulators", MIA_EMULATOR_TARGET, MIA_EMULATOR_TARGET, MIA_EMULATOR_ROM_ROOT, MIA_EMULATOR_SAVE_ROOT, {MIA_EMULATOR_WIDTH, MIA_EMULATOR_HEIGHT}, MIA_EMULATOR_SAMPLE_RATE, NULL, 0};
    MiaCoreStatus status = mia_core_adapter_init(&runtime->adapter, &target, mia_emulator_make_host(runtime));
    if (status.code == MIA_CORE_OK) status = mia_core_adapter_select_rom(&runtime->adapter, runtime->selection.rom_path, runtime->selection.save_name);
    if (status.code == MIA_CORE_OK && !mia_host_audio_open(MIA_EMULATOR_SAMPLE_RATE, 2, 16)) status = mia_core_error(MIA_CORE_ERR_CALLBACK, "audio open failed");
    if (status.code == MIA_CORE_OK) status = mia_emulator_core_boot(runtime);
    if (status.code == MIA_CORE_OK) status = mia_emulator_core_run(runtime);
    if (status.code == MIA_CORE_OK) status = mia_core_adapter_request_exit(&runtime->adapter);
    if (status.code != MIA_CORE_OK) mia_host_log(status.message);
    return status.code == MIA_CORE_OK ? 0 : 1;
}
