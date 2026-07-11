#include "mia_hardware_target.h"

#include <string.h>

static bool valid_id(const char *id) {
    if (id == NULL || id[0] == '\0') {
        return false;
    }
    for (size_t i = 0; id[i] != '\0'; ++i) {
        const char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }
    return true;
}

MiaHardwareStatus mia_hardware_target_find(const MiaHardwareTargetCatalog *catalog, const char *id, const MiaHardwareTarget **out_target) {
    if (catalog == NULL || out_target == NULL || !valid_id(id)) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid target lookup");
    }
    *out_target = NULL;
    for (size_t i = 0; i < catalog->count; ++i) {
        if (strcmp(catalog->targets[i].name, id) == 0) {
            *out_target = &catalog->targets[i];
            return mia_hardware_ok();
        }
    }
    return mia_hardware_error(MIA_HARDWARE_ERR_TARGET_NOT_FOUND, "target not found");
}

bool mia_hardware_target_has_control(const MiaHardwareTarget *target, const char *control) {
    if (target == NULL || control == NULL) {
        return false;
    }
    for (size_t i = 0; i < target->control_count; ++i) {
        if (strcmp(target->controls[i], control) == 0) {
            return true;
        }
    }
    return false;
}
