#include "test_support.h"

#include <cstdlib>
#include <iostream>
#include <new>

void require_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void require_status(MiaRuntimeStatus status, MiaRuntimeStatusCode code) {
    if (status.code != code) {
        std::cerr << "expected status " << code << ", got " << status.code << ": " << status.message << '\n';
        std::exit(1);
    }
}

static MiaRuntimeStatus clock_now(void *ctx, uint64_t *out_us) {
    auto *clock = static_cast<FakeClock *>(ctx);
    if (clock->index >= clock->now_values.size()) {
        return mia_runtime_error(MIA_RUNTIME_ERR_INVALID_ARGUMENT, "fake clock exhausted");
    }
    *out_us = clock->now_values[clock->index++];
    return mia_runtime_ok();
}

static MiaRuntimeStatus clock_wait(void *ctx, uint64_t delay_us) {
    auto *clock = static_cast<FakeClock *>(ctx);
    if (clock->fail_wait) {
        return mia_runtime_error(MIA_RUNTIME_ERR_CLOCK_WAIT_FAILED, "injected wait failure");
    }
    clock->waits.push_back(delay_us);
    return mia_runtime_ok();
}

MiaRuntimeClock fake_clock(FakeClock *clock) {
    return MiaRuntimeClock{clock, clock_now, clock_wait, 0, false};
}

static MiaRuntimeStatus fake_alloc(void *ctx, MiaRuntimeAllocClass alloc_class, size_t size, void **out_ptr) {
    auto *allocator = static_cast<FakeAllocator *>(ctx);
    if (allocator->fail_next) {
        allocator->fail_next = false;
        return mia_runtime_error(MIA_RUNTIME_ERR_OUT_OF_MEMORY, "injected allocation failure");
    }
    *out_ptr = ::operator new(size, std::nothrow);
    if (*out_ptr == nullptr) {
        return mia_runtime_error(MIA_RUNTIME_ERR_OUT_OF_MEMORY, "host allocation failed");
    }
    if (alloc_class == MIA_RUNTIME_ALLOC_INTERNAL) {
        allocator->internal_allocs += 1;
    } else {
        allocator->psram_allocs += 1;
    }
    return mia_runtime_ok();
}

static void fake_free(void *ctx, void *ptr) {
    auto *allocator = static_cast<FakeAllocator *>(ctx);
    allocator->frees += 1;
    ::operator delete(ptr);
}

MiaRuntimeAllocator fake_allocator(FakeAllocator *allocator) {
    return MiaRuntimeAllocator{allocator, fake_alloc, fake_free};
}

static MiaRuntimeStatus lifecycle_step(void *ctx, bool *exit_requested) {
    auto *app = static_cast<FakeLifecycleApp *>(ctx);
    if (app->step_status.code != MIA_RUNTIME_OK) {
        return app->step_status;
    }
    app->steps += 1;
    *exit_requested = app->steps >= app->steps_before_exit;
    return mia_runtime_ok();
}

static MiaRuntimeStatus lifecycle_exit(void *ctx) {
    auto *app = static_cast<FakeLifecycleApp *>(ctx);
    app->exits += 1;
    return mia_runtime_ok();
}

MiaRuntimeLifecycleHooks fake_lifecycle_hooks(FakeLifecycleApp *app) {
    return MiaRuntimeLifecycleHooks{app, lifecycle_step, lifecycle_exit};
}
