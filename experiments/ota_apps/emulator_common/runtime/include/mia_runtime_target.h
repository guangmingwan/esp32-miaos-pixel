#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mia_runtime_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t width;
    uint16_t height;
} MiaRuntimeGeometry;

typedef struct {
    const char *name;
    const char *manifest_category;
    const char *upstream_namespace;
    const char *core_family;
    const char *rom_root;
    const char *save_root;
    MiaRuntimeGeometry geometry;
    uint32_t sample_rate_hz;
    const char *const *aliases;
    size_t alias_count;
} MiaRuntimeTarget;

typedef struct {
    const MiaRuntimeTarget *targets;
    size_t count;
} MiaRuntimeTargetCatalog;

extern const MiaRuntimeTargetCatalog mia_runtime_generated_targets;

MiaRuntimeStatus mia_runtime_target_find(const MiaRuntimeTargetCatalog *catalog, const char *id, const MiaRuntimeTarget **out_target);

#ifdef __cplusplus
}
#endif
