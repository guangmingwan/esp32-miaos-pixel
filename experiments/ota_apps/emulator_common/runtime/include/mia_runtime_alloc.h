#pragma once

#include <stddef.h>

#include "mia_runtime_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIA_RUNTIME_ALLOC_INTERNAL = 0,
    MIA_RUNTIME_ALLOC_PSRAM = 1,
} MiaRuntimeAllocClass;

typedef struct {
    void *ptr;
    size_t size;
    MiaRuntimeAllocClass alloc_class;
} MiaRuntimeAllocation;

typedef MiaRuntimeStatus (*MiaRuntimeAllocFn)(void *ctx, MiaRuntimeAllocClass alloc_class, size_t size, void **out_ptr);
typedef void (*MiaRuntimeFreeFn)(void *ctx, void *ptr);

typedef struct {
    void *ctx;
    MiaRuntimeAllocFn alloc;
    MiaRuntimeFreeFn free;
} MiaRuntimeAllocator;

MiaRuntimeStatus mia_runtime_alloc(const MiaRuntimeAllocator *allocator, MiaRuntimeAllocClass alloc_class, size_t size, MiaRuntimeAllocation *out_allocation);
void mia_runtime_free(const MiaRuntimeAllocator *allocator, MiaRuntimeAllocation *allocation);

#ifdef __cplusplus
}
#endif
