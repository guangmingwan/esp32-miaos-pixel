#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIA_HARDWARE_OK = 0,
    MIA_HARDWARE_ERR_INVALID_ARGUMENT,
    MIA_HARDWARE_ERR_TARGET_NOT_FOUND,
    MIA_HARDWARE_ERR_BUFFER_TOO_SMALL,
    MIA_HARDWARE_ERR_PALETTE_INDEX,
    MIA_HARDWARE_ERR_UNSUPPORTED_RATE,
    MIA_HARDWARE_ERR_QUEUE_FULL,
    MIA_HARDWARE_ERR_QUEUE_EMPTY,
    MIA_HARDWARE_ERR_IO,
} MiaHardwareStatusCode;

typedef struct {
    MiaHardwareStatusCode code;
    const char *message;
} MiaHardwareStatus;

MiaHardwareStatus mia_hardware_ok(void);
MiaHardwareStatus mia_hardware_error(MiaHardwareStatusCode code, const char *message);
bool mia_hardware_status_ok(MiaHardwareStatus status);
const char *mia_hardware_status_code_name(MiaHardwareStatusCode code);

#ifdef __cplusplus
}
#endif
