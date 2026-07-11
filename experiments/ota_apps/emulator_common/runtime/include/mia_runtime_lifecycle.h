#pragma once

#include "mia_runtime_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIA_RUNTIME_LIFECYCLE_READY = 0,
    MIA_RUNTIME_LIFECYCLE_RUNNING,
    MIA_RUNTIME_LIFECYCLE_EXITED,
} MiaRuntimeLifecycleState;

typedef MiaRuntimeStatus (*MiaRuntimeAppStepFn)(void *ctx, bool *exit_requested);
typedef MiaRuntimeStatus (*MiaRuntimeExitFn)(void *ctx);

typedef struct {
    void *ctx;
    MiaRuntimeAppStepFn step;
    MiaRuntimeExitFn clean_exit;
} MiaRuntimeLifecycleHooks;

typedef struct {
    MiaRuntimeLifecycleState state;
    unsigned frame_count;
} MiaRuntimeLifecycle;

MiaRuntimeStatus mia_runtime_lifecycle_init(MiaRuntimeLifecycle *lifecycle);
MiaRuntimeStatus mia_runtime_lifecycle_run(MiaRuntimeLifecycle *lifecycle, MiaRuntimeLifecycleHooks hooks);

#ifdef __cplusplus
}
#endif
