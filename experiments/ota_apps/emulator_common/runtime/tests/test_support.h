#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "mia_runtime_alloc.h"
#include "mia_runtime_lifecycle.h"
#include "mia_runtime_time.h"

void require_true(bool condition, const char *message);
void require_status(MiaRuntimeStatus status, MiaRuntimeStatusCode code);

struct FakeClock {
    std::vector<uint64_t> now_values;
    std::vector<uint64_t> waits;
    bool fail_wait = false;
    std::size_t index = 0;
};

MiaRuntimeClock fake_clock(FakeClock *clock);

struct FakeAllocator {
    bool fail_next = false;
    int internal_allocs = 0;
    int psram_allocs = 0;
    int frees = 0;
};

MiaRuntimeAllocator fake_allocator(FakeAllocator *allocator);

struct FakeLifecycleApp {
    int steps_before_exit = 0;
    int steps = 0;
    int exits = 0;
    MiaRuntimeStatus step_status = {MIA_RUNTIME_OK, "ok"};
};

MiaRuntimeLifecycleHooks fake_lifecycle_hooks(FakeLifecycleApp *app);
