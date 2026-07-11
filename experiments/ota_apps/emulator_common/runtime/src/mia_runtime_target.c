#include "mia_runtime_target.h"

#include <string.h>

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

static bool target_matches(const MiaRuntimeTarget *target, const char *id) {
    if (strcmp(target->name, id) == 0 || strcmp(target->upstream_namespace, id) == 0) {
        return true;
    }
    for (size_t index = 0; index < target->alias_count; ++index) {
        if (strcmp(target->aliases[index], id) == 0) {
            return true;
        }
    }
    return false;
}

MiaRuntimeStatus mia_runtime_target_find(const MiaRuntimeTargetCatalog *catalog, const char *id, const MiaRuntimeTarget **out_target) {
    if (catalog == NULL || catalog->targets == NULL || out_target == NULL || !valid_id(id)) {
        return mia_runtime_error(MIA_RUNTIME_ERR_INVALID_ARGUMENT, "valid target id is required");
    }
    *out_target = NULL;
    for (size_t index = 0; index < catalog->count; ++index) {
        const MiaRuntimeTarget *target = &catalog->targets[index];
        if (target_matches(target, id)) {
            *out_target = target;
            return mia_runtime_ok();
        }
    }
    return mia_runtime_error(MIA_RUNTIME_ERR_TARGET_NOT_FOUND, "target id was not found");
}
