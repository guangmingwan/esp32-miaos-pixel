#include "mia_app_save.h"

#include <stdio.h>
#include <string.h>

static MiaStorageStatus derive_save_name(const char *rom_path, char *out_name, size_t out_size) {
    const char *base = strrchr(rom_path, '/');
    base = base == NULL ? rom_path : base + 1;
    const char *dot = strrchr(base, '.');
    const size_t stem_len = dot == NULL ? strlen(base) : (size_t)(dot - base);
    if (stem_len == 0 || stem_len + 5u > out_size) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "save name is too long");
    }
    memcpy(out_name, base, stem_len);
    memcpy(out_name + stem_len, ".sav", 5);
    return mia_storage_ok();
}

MiaStorageStatus mia_app_picker_select_entry(const MiaStoragePickerResult *result, size_t index, MiaAppPickerSelection *out_selection) {
    if (result == NULL || out_selection == NULL || index >= result->count || result->entries[index].kind != MIA_STORAGE_ENTRY_ROM) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "explicit ROM selection is required");
    }
    *out_selection = (MiaAppPickerSelection){0};
    if (snprintf(out_selection->rom_path, sizeof(out_selection->rom_path), "%s", result->entries[index].path) >= (int)sizeof(out_selection->rom_path)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "ROM path is too long");
    }
    return derive_save_name(result->entries[index].path, out_selection->save_name, sizeof(out_selection->save_name));
}

void mia_app_picker_selection_free(MiaAppPickerSelection *selection) {
    if (selection != NULL) {
        *selection = (MiaAppPickerSelection){0};
    }
}

MiaStorageStatus mia_app_save_flush(const MiaStorageContext *context, const MiaStorageTarget *target, const char *save_name, MiaStorageFlushReason reason, const uint8_t *data, size_t size, const MiaStorageFault *fault) {
    const MiaStorageSaveRequest request = {target, save_name, reason, fault};
    return mia_storage_save_write(context, &request, data, size);
}

MiaStorageStatus mia_app_save_load(const MiaStorageContext *context, const MiaStorageTarget *target, const char *save_name, uint8_t *data, size_t capacity, size_t *out_size) {
    return mia_storage_save_read(context, target, save_name, data, capacity, out_size);
}
