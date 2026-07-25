#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"
#include "launch_context.h"

#include <esp_system.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef MIA_EMULATOR_TARGET
#error "MIA_EMULATOR_TARGET is required"
#endif

#ifndef MIA_EMULATOR_SCALE_MODE
#define MIA_EMULATOR_SCALE_MODE MIA_DISPLAY_SCALE_FIT
#endif

static const char *const extensions[] = {
#ifdef MIA_EMULATOR_GNUBOY
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
#ifdef MIA_EMULATOR_FIFTH_EXTENSION
    MIA_EMULATOR_FIFTH_EXTENSION,
#endif
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
#ifdef MIA_EMULATOR_BBK
    MIA_EMULATOR_EXTENSION,
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
    runtime->display = calloc(MIA_DISPLAY_PIXELS, sizeof(uint16_t));
    runtime->audio_queue = malloc(4096u * sizeof(int16_t));
    runtime->audio_drain = malloc(1024u * sizeof(int16_t));
    return runtime->display != NULL && runtime->audio_queue != NULL && runtime->audio_drain != NULL;
}

static MiaStorageStatus direct_selection(const char *path, MiaAppPickerSelection *selection) {
    char physical_path[MIA_APP_PATH_MAX];
    const char *selected_path = path;
    if (strncmp(path, "/sd/", 4) != 0 && strcmp(path, "/sd") != 0) {
        if (snprintf(physical_path, sizeof(physical_path), "/sd%s", path) >=
            (int)sizeof(physical_path)) {
            return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "direct ROM path is too long");
        }
        selected_path = physical_path;
    }

    const char *base = strrchr(selected_path, '/');
    base = base == NULL ? selected_path : base + 1;
    const char *dot = strrchr(base, '.');
    const size_t stem_length = dot == NULL ? strlen(base) : (size_t)(dot - base);
    if (stem_length == 0 || stem_length + 5u > sizeof(selection->save_name) ||
        snprintf(selection->rom_path, sizeof(selection->rom_path), "%s", selected_path) >=
            (int)sizeof(selection->rom_path)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "direct ROM path is too long");
    }
    memcpy(selection->save_name, base, stem_length);
    memcpy(selection->save_name + stem_length, ".sav", 5);
    return mia_storage_ok();
}

static bool state_paths(MiaEmulatorRuntime *runtime, char *path, char *temporary,
                        char *backup, size_t path_size) {
    const char *save_name = runtime->selection.save_name;
    const char *dot = strrchr(save_name, '.');
    const size_t stem_length = dot == NULL ? strlen(save_name) : (size_t)(dot - save_name);
    char save_root[MIA_APP_PATH_MAX];
    if (stem_length == 0 || snprintf(save_root, sizeof(save_root), "%s%s",
                                    runtime->storage.storage_root,
                                    runtime->storage_target.save_root) >= (int)sizeof(save_root) ||
        snprintf(path, path_size, "%s/%.*s.state%u", save_root, (int)stem_length,
                 save_name, runtime->state_slot) >= (int)path_size ||
        snprintf(temporary, path_size, "%s.tmp", path) >= (int)path_size ||
        snprintf(backup, path_size, "%s.bak", path) >= (int)path_size) {
        return false;
    }
    (void)mkdir("/sd/saves", 0775);
    (void)mkdir(save_root, 0775);
    return true;
}

static MiaCoreStatus save_state_slot(MiaEmulatorRuntime *runtime) {
    char path[MIA_APP_PATH_MAX];
    char temporary[MIA_APP_PATH_MAX];
    char backup[MIA_APP_PATH_MAX];
    if (!state_paths(runtime, path, temporary, backup, sizeof(path))) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "state path is too long");
    }
    (void)remove(temporary);
    MiaCoreStatus status = mia_emulator_core_save_state(runtime, temporary);
    if (status.code != MIA_CORE_OK) {
        (void)remove(temporary);
        return status;
    }
    (void)remove(backup);
    const bool had_previous = rename(path, backup) == 0;
    if (!had_previous && errno != ENOENT) {
        (void)remove(temporary);
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "failed to stage previous state");
    }
    if (rename(temporary, path) != 0) {
        if (had_previous) (void)rename(backup, path);
        (void)remove(temporary);
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "failed to replace state");
    }
    if (had_previous) (void)remove(backup);
    return mia_core_ok();
}

static MiaCoreStatus load_state_slot(MiaEmulatorRuntime *runtime) {
    char path[MIA_APP_PATH_MAX];
    char temporary[MIA_APP_PATH_MAX];
    char backup[MIA_APP_PATH_MAX];
    if (!state_paths(runtime, path, temporary, backup, sizeof(path))) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "state path is too long");
    }
    if (access(path, F_OK) != 0) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "state slot is empty");
    }
    return mia_emulator_core_load_state(runtime, path);
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
    mia_app_input_init(&runtime->input);
    (void)mia_app_audio_init(&runtime->audio, runtime->audio_queue, 2048);
    char direct_path[MIA_HOST_LAUNCH_ARG_SIZE];
    MiaStorageStatus pick;
    if (mia_host_consume_launch_arg(MIA_APP_NAME, direct_path, sizeof(direct_path))) {
        char direct_log[MIA_HOST_LAUNCH_ARG_SIZE + 32];
        snprintf(direct_log, sizeof(direct_log), "direct launch: %s", direct_path);
        mia_host_log(direct_log);
        pick = direct_selection(direct_path, &runtime->selection);
    } else {
        pick = mia_emulator_picker_run(&runtime->storage, &runtime->storage_target, &runtime->selection);
    }
    if (pick.code != MIA_STORAGE_OK) return 1;
    const MiaRuntimeTarget target = {MIA_EMULATOR_TARGET, "Emulators", MIA_EMULATOR_TARGET, MIA_EMULATOR_TARGET, MIA_EMULATOR_ROM_ROOT, MIA_EMULATOR_SAVE_ROOT, {MIA_EMULATOR_WIDTH, MIA_EMULATOR_HEIGHT}, MIA_EMULATOR_SAMPLE_RATE, NULL, 0};
    MiaCoreStatus status = mia_core_adapter_init(&runtime->adapter, &target, mia_emulator_make_host(runtime));
    if (status.code == MIA_CORE_OK) status = mia_core_adapter_select_rom(&runtime->adapter, runtime->selection.rom_path, runtime->selection.save_name);
    if (status.code == MIA_CORE_OK && !mia_host_audio_open(MIA_EMULATOR_SAMPLE_RATE, 2, 16)) status = mia_core_error(MIA_CORE_ERR_CALLBACK, "audio open failed");
    if (status.code == MIA_CORE_OK) status = mia_emulator_core_boot(runtime);
    if (status.code == MIA_CORE_OK) mia_emulator_wait_input_release(runtime);
    while (status.code == MIA_CORE_OK) {
        status = mia_emulator_core_run(runtime);
        if (status.code != MIA_CORE_OK) break;
        mia_host_audio_stop();
        MiaEmulatorMenuNotice notice = MIA_EMULATOR_MENU_NOTICE_NONE;
        bool resume = false;
        for (;;) {
            const MiaEmulatorMenuAction action = mia_emulator_menu_run(
                MIA_EMULATOR_TARGET, runtime->display, notice, &runtime->state_slot,
                &runtime->video.scale_mode);
            notice = MIA_EMULATOR_MENU_NOTICE_NONE;
            if (action == MIA_EMULATOR_MENU_RESUME) {
                resume = true;
                break;
            }
            if (action == MIA_EMULATOR_MENU_SAVE_STATE ||
                action == MIA_EMULATOR_MENU_LOAD_STATE) {
                MiaCoreStatus state_status = action == MIA_EMULATOR_MENU_SAVE_STATE
                    ? save_state_slot(runtime) : load_state_slot(runtime);
                if (state_status.code == MIA_CORE_OK) {
                    if (action == MIA_EMULATOR_MENU_LOAD_STATE) {
                        resume = true;
                        break;
                    }
                    notice = MIA_EMULATOR_MENU_NOTICE_STATE_SAVED;
                } else if (state_status.code == MIA_CORE_ERR_UNSUPPORTED) {
                    notice = MIA_EMULATOR_MENU_NOTICE_STATE_UNSUPPORTED;
                } else if (strcmp(state_status.message, "state slot is empty") == 0) {
                    notice = MIA_EMULATOR_MENU_NOTICE_STATE_MISSING;
                } else {
                    notice = MIA_EMULATOR_MENU_NOTICE_STATE_ERROR;
                }
                mia_host_log(state_status.message);
                continue;
            }
            status = mia_core_adapter_request_exit(&runtime->adapter);
            if (status.code == MIA_CORE_OK && action == MIA_EMULATOR_MENU_ROM_PICKER) {
                mia_host_log("returning to ROM picker");
                esp_restart();
            }
            break;
        }
        if (resume) {
            mia_host_fill_screen_rgb565(0);
            mia_emulator_wait_input_release(runtime);
            continue;
        }
        break;
    }
    if (status.code != MIA_CORE_OK) mia_host_log(status.message);
    return status.code == MIA_CORE_OK ? 0 : 1;
}
