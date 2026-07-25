#pragma once

#include "mia_app_audio.h"
#include "mia_app_input.h"
#include "mia_app_save.h"
#include "mia_app_video.h"
#include "mia_emulator_core.h"

typedef struct {
    MiaCoreAdapter adapter;
    MiaAppAudioSink audio;
    MiaAppInputState input;
    MiaAppVideoSink video;
    MiaStorageContext storage;
    MiaStorageTarget storage_target;
    MiaHardwareTarget hardware_target;
    MiaAppPickerSelection selection;
    uint16_t *display;
    uint8_t state_slot;
    int16_t *audio_queue;
    int16_t *audio_drain;
} MiaEmulatorRuntime;

typedef enum {
    MIA_EMULATOR_MENU_RESUME = 0,
    MIA_EMULATOR_MENU_SAVE_STATE,
    MIA_EMULATOR_MENU_LOAD_STATE,
    MIA_EMULATOR_MENU_ROM_PICKER,
    MIA_EMULATOR_MENU_EXIT_LAUNCHER,
} MiaEmulatorMenuAction;

typedef enum {
    MIA_EMULATOR_MENU_NOTICE_NONE = 0,
    MIA_EMULATOR_MENU_NOTICE_STATE_SAVED,
    MIA_EMULATOR_MENU_NOTICE_STATE_MISSING,
    MIA_EMULATOR_MENU_NOTICE_STATE_ERROR,
    MIA_EMULATOR_MENU_NOTICE_STATE_UNSUPPORTED,
} MiaEmulatorMenuNotice;

extern MiaEmulatorRuntime mia_emulator_runtime;

uint32_t mia_emulator_host_buttons(void);
MiaCoreHost mia_emulator_make_host(MiaEmulatorRuntime *runtime);
MiaStorageStatus mia_emulator_picker_run(const MiaStorageContext *context, const MiaStorageTarget *target, MiaAppPickerSelection *selection);
MiaEmulatorMenuAction mia_emulator_menu_run(const char *target_name, uint16_t *screenshot,
                                             MiaEmulatorMenuNotice notice, uint8_t *state_slot,
                                             MiaDisplayScaleMode *scale_mode);
MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime);
void mia_emulator_wait_input_release(MiaEmulatorRuntime *runtime);
MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime);
MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force);
MiaCoreStatus mia_emulator_core_save_state(MiaEmulatorRuntime *runtime, const char *path);
MiaCoreStatus mia_emulator_core_load_state(MiaEmulatorRuntime *runtime, const char *path);
