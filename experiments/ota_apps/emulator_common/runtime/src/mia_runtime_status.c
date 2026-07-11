#include "mia_runtime_status.h"

MiaRuntimeStatus mia_runtime_ok(void) {
    return (MiaRuntimeStatus){MIA_RUNTIME_OK, "ok"};
}

MiaRuntimeStatus mia_runtime_error(MiaRuntimeStatusCode code, const char *message) {
    return (MiaRuntimeStatus){code, message};
}

bool mia_runtime_status_ok(MiaRuntimeStatus status) {
    return status.code == MIA_RUNTIME_OK;
}

const char *mia_runtime_status_code_name(MiaRuntimeStatusCode code) {
    switch (code) {
    case MIA_RUNTIME_OK:
        return "ok";
    case MIA_RUNTIME_ERR_INVALID_ARGUMENT:
        return "invalid-argument";
    case MIA_RUNTIME_ERR_CLOCK_BACKWARDS:
        return "clock-backwards";
    case MIA_RUNTIME_ERR_CLOCK_WAIT_FAILED:
        return "clock-wait-failed";
    case MIA_RUNTIME_ERR_OUT_OF_MEMORY:
        return "out-of-memory";
    case MIA_RUNTIME_ERR_LIFECYCLE:
        return "lifecycle";
    case MIA_RUNTIME_ERR_TARGET_NOT_FOUND:
        return "target-not-found";
    }
    return "unknown";
}
