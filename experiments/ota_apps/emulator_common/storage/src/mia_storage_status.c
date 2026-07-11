#include "mia_storage.h"

#include <ctype.h>
#include <string.h>

MiaStorageStatus mia_storage_ok(void) {
    return (MiaStorageStatus){MIA_STORAGE_OK, "ok"};
}

MiaStorageStatus mia_storage_error(MiaStorageStatusCode code, const char *message) {
    return (MiaStorageStatus){code, message};
}

const char *mia_storage_status_code_name(MiaStorageStatusCode code) {
    switch (code) {
    case MIA_STORAGE_OK:
        return "ok";
    case MIA_STORAGE_ERR_INVALID_ARGUMENT:
        return "invalid-argument";
    case MIA_STORAGE_ERR_TARGET_NOT_FOUND:
        return "target-not-found";
    case MIA_STORAGE_ERR_MISSING_ROOT:
        return "missing-root";
    case MIA_STORAGE_ERR_PATH_TRAVERSAL:
        return "path-traversal";
    case MIA_STORAGE_ERR_UNSUPPORTED_ARCHIVE:
        return "unsupported-archive";
    case MIA_STORAGE_ERR_UNSUPPORTED_FILE:
        return "unsupported-file";
    case MIA_STORAGE_ERR_MISSING_REQUIRED_FILE:
        return "missing-required-file";
    case MIA_STORAGE_ERR_IO:
        return "io";
    case MIA_STORAGE_ERR_INTERRUPTED:
        return "interrupted";
    }
    return "unknown";
}

static bool valid_id(const char *id) {
    if (id == NULL || id[0] == '\0') {
        return false;
    }
    for (const char *cursor = id; *cursor != '\0'; ++cursor) {
        const char ch = *cursor;
        const bool valid = (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        if (!valid) {
            return false;
        }
    }
    return true;
}

MiaStorageStatus mia_storage_target_find(const MiaStorageTargetCatalog *catalog, const char *id, const MiaStorageTarget **out_target) {
    if (catalog == NULL || catalog->targets == NULL || out_target == NULL || !valid_id(id)) {
        return mia_storage_error(MIA_STORAGE_ERR_INVALID_ARGUMENT, "valid target id is required");
    }
    *out_target = NULL;
    for (size_t index = 0; index < catalog->count; ++index) {
        const MiaStorageTarget *target = &catalog->targets[index];
        if (strcmp(target->name, id) == 0) {
            *out_target = target;
            return mia_storage_ok();
        }
    }
    return mia_storage_error(MIA_STORAGE_ERR_TARGET_NOT_FOUND, "target id was not found");
}
