#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mia_runtime_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef MiaRuntimeStatus (*MiaRuntimeClockNowFn)(void *ctx, uint64_t *out_us);
typedef MiaRuntimeStatus (*MiaRuntimeClockWaitFn)(void *ctx, uint64_t delay_us);

typedef struct {
    void *ctx;
    MiaRuntimeClockNowFn now_us;
    MiaRuntimeClockWaitFn wait_us;
    uint64_t last_us;
    bool seen;
} MiaRuntimeClock;

typedef struct {
    MiaRuntimeClock clock;
    uint64_t frame_interval_us;
    uint64_t next_frame_us;
    uint64_t last_now_us;
    bool started;
} MiaRuntimeFramePacer;

MiaRuntimeStatus mia_runtime_clock_now(MiaRuntimeClock *clock, uint64_t *out_us);
MiaRuntimeStatus mia_runtime_frame_pacer_init(MiaRuntimeFramePacer *pacer, MiaRuntimeClock clock, uint32_t frame_rate_hz);
MiaRuntimeStatus mia_runtime_frame_pacer_step(MiaRuntimeFramePacer *pacer, uint64_t *waited_us);

#ifdef __cplusplus
}
#endif
