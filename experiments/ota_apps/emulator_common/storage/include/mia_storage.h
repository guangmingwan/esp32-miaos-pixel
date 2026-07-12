#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIA_STORAGE_OK = 0,
    MIA_STORAGE_ERR_INVALID_ARGUMENT,
    MIA_STORAGE_ERR_TARGET_NOT_FOUND,
    MIA_STORAGE_ERR_MISSING_ROOT,
    MIA_STORAGE_ERR_PATH_TRAVERSAL,
    MIA_STORAGE_ERR_UNSUPPORTED_ARCHIVE,
    MIA_STORAGE_ERR_UNSUPPORTED_FILE,
    MIA_STORAGE_ERR_MISSING_REQUIRED_FILE,
    MIA_STORAGE_ERR_IO,
    MIA_STORAGE_ERR_INTERRUPTED,
} MiaStorageStatusCode;

typedef struct {
    MiaStorageStatusCode code;
    const char *message;
} MiaStorageStatus;

typedef enum {
    MIA_STORAGE_ENTRY_DIRECTORY = 1,
    MIA_STORAGE_ENTRY_ROM,
} MiaStorageEntryKind;

typedef enum {
    MIA_STORAGE_FLUSH_CORE_REQUEST = 1,
    MIA_STORAGE_FLUSH_ROM_CHANGE,
    MIA_STORAGE_FLUSH_CLEAN_EXIT,
} MiaStorageFlushReason;

typedef enum {
    MIA_STORAGE_FAULT_NONE = 0,
    MIA_STORAGE_FAULT_BEFORE_REPLACE,
    MIA_STORAGE_FAULT_WRITE_FULL,
    MIA_STORAGE_FAULT_RENAME_NO_REPLACE,
} MiaStorageFaultKind;

typedef struct {
    const char *kind;
    const char *path;
    bool required;
} MiaStorageRequirement;

typedef struct {
    const char *name;
    const char *rom_root;
    const char *save_root;
    const char *const *extensions;
    size_t extension_count;
    const MiaStorageRequirement *requirements;
    size_t requirement_count;
} MiaStorageTarget;

typedef struct {
    const MiaStorageTarget *targets;
    size_t count;
} MiaStorageTargetCatalog;

typedef struct {
    const char *storage_root;
} MiaStorageContext;

typedef struct {
    char *name;
    char *path;
    MiaStorageEntryKind kind;
    uint64_t size;
} MiaStoragePickerEntry;

typedef struct {
    MiaStoragePickerEntry *entries;
    size_t count;
} MiaStoragePickerResult;

MiaStorageStatus mia_storage_rom_root_path(const MiaStorageContext *context,
                                           const MiaStorageTarget *target,
                                           char *out_path, size_t out_size);

typedef struct {
    MiaStorageFaultKind kind;
} MiaStorageFault;

typedef struct {
    const MiaStorageTarget *target;
    const char *save_name;
    MiaStorageFlushReason reason;
    const MiaStorageFault *fault;
} MiaStorageSaveRequest;

extern const MiaStorageTargetCatalog mia_storage_generated_targets;

MiaStorageStatus mia_storage_ok(void);
MiaStorageStatus mia_storage_error(MiaStorageStatusCode code, const char *message);
const char *mia_storage_status_code_name(MiaStorageStatusCode code);
MiaStorageStatus mia_storage_target_find(const MiaStorageTargetCatalog *catalog, const char *id, const MiaStorageTarget **out_target);
MiaStorageStatus mia_storage_picker_list(const MiaStorageContext *context, const MiaStorageTarget *target, MiaStoragePickerResult *out_result);
MiaStorageStatus mia_storage_picker_select(const MiaStorageContext *context, const MiaStorageTarget *target, const char *relative_name, MiaStoragePickerResult *out_result);
void mia_storage_picker_free(MiaStoragePickerResult *result);
MiaStorageStatus mia_storage_validate_requirements(const MiaStorageContext *context, const MiaStorageTarget *target);
MiaStorageStatus mia_storage_save_write(const MiaStorageContext *context, const MiaStorageSaveRequest *request, const uint8_t *data, size_t size);
MiaStorageStatus mia_storage_save_read(const MiaStorageContext *context, const MiaStorageTarget *target, const char *save_name, uint8_t *data, size_t capacity, size_t *out_size);

#ifdef __cplusplus
}
#endif
