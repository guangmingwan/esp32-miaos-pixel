#pragma once

#include "mia_storage.h"

#include <stdbool.h>
#include <stddef.h>

#define MIA_STORAGE_PATH_MAX 4096

typedef struct {
    char path[MIA_STORAGE_PATH_MAX];
} MiaStoragePath;

MiaStorageStatus mia_storage_resolve_virtual(const MiaStorageContext *context, const char *virtual_path, MiaStoragePath *out_path);
MiaStorageStatus mia_storage_resolve_child(const MiaStorageContext *context, const char *virtual_root, const char *relative_name, MiaStoragePath *out_path);
MiaStorageStatus mia_storage_prepare_child(const MiaStorageContext *context, const char *virtual_root, const char *relative_name, MiaStoragePath *out_path);
MiaStorageStatus mia_storage_make_parents(const char *path);
bool mia_storage_extension_matches(const char *name, const MiaStorageTarget *target);
bool mia_storage_is_zip(const char *name);
