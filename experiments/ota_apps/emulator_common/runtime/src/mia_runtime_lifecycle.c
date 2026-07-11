#include "mia_runtime_lifecycle.h"

#include <stddef.h>

MiaRuntimeStatus mia_runtime_lifecycle_init(MiaRuntimeLifecycle *lifecycle) {
    if (lifecycle == NULL) {
        return mia_runtime_error(MIA_RUNTIME_ERR_INVALID_ARGUMENT, "lifecycle is required");
    }
    *lifecycle = (MiaRuntimeLifecycle){MIA_RUNTIME_LIFECYCLE_READY, 0U};
    return mia_runtime_ok();
}

MiaRuntimeStatus mia_runtime_lifecycle_run(MiaRuntimeLifecycle *lifecycle, MiaRuntimeLifecycleHooks hooks) {
    if (lifecycle == NULL || hooks.step == NULL || hooks.clean_exit == NULL) {
        return mia_runtime_error(MIA_RUNTIME_ERR_INVALID_ARGUMENT, "lifecycle hooks are required");
    }
    if (lifecycle->state != MIA_RUNTIME_LIFECYCLE_READY) {
        return mia_runtime_error(MIA_RUNTIME_ERR_LIFECYCLE, "lifecycle must be ready before run");
    }
    lifecycle->state = MIA_RUNTIME_LIFECYCLE_RUNNING;
    bool exit_requested = false;
    while (!exit_requested) {
        MiaRuntimeStatus status = hooks.step(hooks.ctx, &exit_requested);
        if (!mia_runtime_status_ok(status)) {
            return status;
        }
        lifecycle->frame_count += 1U;
    }
    MiaRuntimeStatus status = hooks.clean_exit(hooks.ctx);
    if (!mia_runtime_status_ok(status)) {
        return status;
    }
    lifecycle->state = MIA_RUNTIME_LIFECYCLE_EXITED;
    return mia_runtime_ok();
}
