#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIA_RUNTIME_OK = 0,
    MIA_RUNTIME_ERR_INVALID_ARGUMENT,
    MIA_RUNTIME_ERR_CLOCK_BACKWARDS,
    MIA_RUNTIME_ERR_CLOCK_WAIT_FAILED,
    MIA_RUNTIME_ERR_OUT_OF_MEMORY,
    MIA_RUNTIME_ERR_LIFECYCLE,
    MIA_RUNTIME_ERR_TARGET_NOT_FOUND,
} MiaRuntimeStatusCode;

typedef struct {
    MiaRuntimeStatusCode code;
    const char *message;
} MiaRuntimeStatus;

MiaRuntimeStatus mia_runtime_ok(void);
MiaRuntimeStatus mia_runtime_error(MiaRuntimeStatusCode code, const char *message);
bool mia_runtime_status_ok(MiaRuntimeStatus status);
const char *mia_runtime_status_code_name(MiaRuntimeStatusCode code);

#ifdef __cplusplus
}
#endif
