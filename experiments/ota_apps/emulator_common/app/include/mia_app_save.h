#pragma once

#include "mia_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIA_APP_PATH_MAX
#define MIA_APP_PATH_MAX 256
#endif

typedef struct {
    char rom_path[MIA_APP_PATH_MAX];
    char save_name[MIA_APP_PATH_MAX];
} MiaAppPickerSelection;

MiaStorageStatus mia_app_picker_select_entry(const MiaStoragePickerResult *result, size_t index, MiaAppPickerSelection *out_selection);
void mia_app_picker_selection_free(MiaAppPickerSelection *selection);
MiaStorageStatus mia_app_save_flush(const MiaStorageContext *context, const MiaStorageTarget *target, const char *save_name, MiaStorageFlushReason reason, const uint8_t *data, size_t size, const MiaStorageFault *fault);
MiaStorageStatus mia_app_save_load(const MiaStorageContext *context, const MiaStorageTarget *target, const char *save_name, uint8_t *data, size_t capacity, size_t *out_size);

#ifdef __cplusplus
}
#endif
