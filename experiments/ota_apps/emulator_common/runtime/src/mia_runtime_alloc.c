#include "mia_runtime_alloc.h"

MiaRuntimeStatus mia_runtime_alloc(const MiaRuntimeAllocator *allocator, MiaRuntimeAllocClass alloc_class, size_t size, MiaRuntimeAllocation *out_allocation) {
    if (allocator == NULL || allocator->alloc == NULL || out_allocation == NULL || size == 0U) {
        return mia_runtime_error(MIA_RUNTIME_ERR_INVALID_ARGUMENT, "allocator arguments are required");
    }
    *out_allocation = (MiaRuntimeAllocation){0};
    void *ptr = NULL;
    MiaRuntimeStatus status = allocator->alloc(allocator->ctx, alloc_class, size, &ptr);
    if (!mia_runtime_status_ok(status)) {
        return status;
    }
    if (ptr == NULL) {
        return mia_runtime_error(MIA_RUNTIME_ERR_OUT_OF_MEMORY, "allocator returned null");
    }
    *out_allocation = (MiaRuntimeAllocation){ptr, size, alloc_class};
    return mia_runtime_ok();
}

void mia_runtime_free(const MiaRuntimeAllocator *allocator, MiaRuntimeAllocation *allocation) {
    if (allocator == NULL || allocator->free == NULL || allocation == NULL || allocation->ptr == NULL) {
        return;
    }
    allocator->free(allocator->ctx, allocation->ptr);
    *allocation = (MiaRuntimeAllocation){0};
}
