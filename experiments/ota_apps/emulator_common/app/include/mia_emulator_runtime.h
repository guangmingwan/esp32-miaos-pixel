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
    int16_t *audio_queue;
    int16_t *audio_drain;
} MiaEmulatorRuntime;

extern MiaEmulatorRuntime mia_emulator_runtime;

uint32_t mia_emulator_host_buttons(void);
MiaCoreHost mia_emulator_make_host(MiaEmulatorRuntime *runtime);
MiaStorageStatus mia_emulator_picker_run(const MiaStorageContext *context, const MiaStorageTarget *target, MiaAppPickerSelection *selection);
MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime);
MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime);
MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force);
