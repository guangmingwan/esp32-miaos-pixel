#include "mia_hardware_status.h"

MiaHardwareStatus mia_hardware_ok(void) {
    return (MiaHardwareStatus){MIA_HARDWARE_OK, "ok"};
}

MiaHardwareStatus mia_hardware_error(MiaHardwareStatusCode code, const char *message) {
    return (MiaHardwareStatus){code, message};
}

bool mia_hardware_status_ok(MiaHardwareStatus status) {
    return status.code == MIA_HARDWARE_OK;
}

const char *mia_hardware_status_code_name(MiaHardwareStatusCode code) {
    switch (code) {
    case MIA_HARDWARE_OK:
        return "ok";
    case MIA_HARDWARE_ERR_INVALID_ARGUMENT:
        return "invalid-argument";
    case MIA_HARDWARE_ERR_TARGET_NOT_FOUND:
        return "target-not-found";
    case MIA_HARDWARE_ERR_BUFFER_TOO_SMALL:
        return "buffer-too-small";
    case MIA_HARDWARE_ERR_PALETTE_INDEX:
        return "palette-index";
    case MIA_HARDWARE_ERR_UNSUPPORTED_RATE:
        return "unsupported-rate";
    case MIA_HARDWARE_ERR_QUEUE_FULL:
        return "queue-full";
    case MIA_HARDWARE_ERR_QUEUE_EMPTY:
        return "queue-empty";
    case MIA_HARDWARE_ERR_IO:
        return "io";
    }
    return "unknown";
}
