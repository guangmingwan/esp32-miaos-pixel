#include "mia_runtime_time.h"

#include <stddef.h>

MiaRuntimeStatus mia_runtime_clock_now(MiaRuntimeClock *clock, uint64_t *out_us) {
    if (clock == NULL || clock->now_us == NULL || out_us == NULL) {
        return mia_runtime_error(MIA_RUNTIME_ERR_INVALID_ARGUMENT, "clock arguments are required");
    }
    uint64_t now = 0;
    MiaRuntimeStatus status = clock->now_us(clock->ctx, &now);
    if (!mia_runtime_status_ok(status)) {
        return status;
    }
    if (clock->seen && now < clock->last_us) {
        return mia_runtime_error(MIA_RUNTIME_ERR_CLOCK_BACKWARDS, "monotonic clock moved backwards");
    }
    clock->seen = true;
    clock->last_us = now;
    *out_us = now;
    return mia_runtime_ok();
}

MiaRuntimeStatus mia_runtime_frame_pacer_init(MiaRuntimeFramePacer *pacer, MiaRuntimeClock clock, uint32_t frame_rate_hz) {
    if (pacer == NULL || frame_rate_hz == 0U) {
        return mia_runtime_error(MIA_RUNTIME_ERR_INVALID_ARGUMENT, "pacer and frame rate are required");
    }
    *pacer = (MiaRuntimeFramePacer){0};
    pacer->clock = clock;
    pacer->frame_interval_us = 1000000ULL / frame_rate_hz;
    return mia_runtime_ok();
}

MiaRuntimeStatus mia_runtime_frame_pacer_step(MiaRuntimeFramePacer *pacer, uint64_t *waited_us) {
    if (pacer == NULL || waited_us == NULL) {
        return mia_runtime_error(MIA_RUNTIME_ERR_INVALID_ARGUMENT, "pacer arguments are required");
    }
    uint64_t now = 0;
    MiaRuntimeStatus status = mia_runtime_clock_now(&pacer->clock, &now);
    if (!mia_runtime_status_ok(status)) {
        return status;
    }
    if (!pacer->started) {
        pacer->started = true;
        pacer->next_frame_us = now + pacer->frame_interval_us;
        *waited_us = 0;
        return mia_runtime_ok();
    }
    uint64_t delay_us = now < pacer->next_frame_us ? pacer->next_frame_us - now : 0;
    if (delay_us > 0U && pacer->clock.wait_us != NULL) {
        status = pacer->clock.wait_us(pacer->clock.ctx, delay_us);
        if (!mia_runtime_status_ok(status)) {
            return status;
        }
    }
    pacer->next_frame_us += pacer->frame_interval_us;
    *waited_us = delay_us;
    return mia_runtime_ok();
}
