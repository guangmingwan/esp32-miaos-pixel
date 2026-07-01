#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t mia_host_abi_version(void);
void mia_host_log(const char *message);

#ifdef __cplusplus
}
#endif
