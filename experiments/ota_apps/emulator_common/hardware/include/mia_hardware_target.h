#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mia_hardware_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t width;
    uint16_t height;
} MiaHardwareGeometry;

typedef struct {
    const char *name;
    MiaHardwareGeometry geometry;
    uint32_t sample_rate_hz;
    const char *const *controls;
    size_t control_count;
} MiaHardwareTarget;

typedef struct {
    const MiaHardwareTarget *targets;
    size_t count;
} MiaHardwareTargetCatalog;

extern const MiaHardwareTargetCatalog mia_hardware_generated_targets;

MiaHardwareStatus mia_hardware_target_find(const MiaHardwareTargetCatalog *catalog, const char *id, const MiaHardwareTarget **out_target);
bool mia_hardware_target_has_control(const MiaHardwareTarget *target, const char *control);

#ifdef __cplusplus
}
#endif
