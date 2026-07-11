#include "test_support.h"

int main() {
    FakeAllocator state{};
    MiaRuntimeAllocator allocator = fake_allocator(&state);
    MiaRuntimeAllocation internal{};
    MiaRuntimeAllocation psram{};

    require_status(mia_runtime_alloc(&allocator, MIA_RUNTIME_ALLOC_INTERNAL, 64, &internal), MIA_RUNTIME_OK);
    require_status(mia_runtime_alloc(&allocator, MIA_RUNTIME_ALLOC_PSRAM, 128, &psram), MIA_RUNTIME_OK);
    require_true(state.internal_allocs == 1, "internal allocation count should be tracked");
    require_true(state.psram_allocs == 1, "psram allocation count should be tracked");
    require_true(internal.size == 64 && psram.size == 128, "allocation metadata should preserve sizes");

    state.fail_next = true;
    MiaRuntimeAllocation failed{};
    require_status(mia_runtime_alloc(&allocator, MIA_RUNTIME_ALLOC_PSRAM, 32, &failed), MIA_RUNTIME_ERR_OUT_OF_MEMORY);
    require_true(failed.ptr == nullptr, "failed allocation should not expose stale pointers");

    mia_runtime_free(&allocator, &internal);
    mia_runtime_free(&allocator, &psram);
    require_true(state.frees == 2, "free should release successful allocations exactly once");
}
