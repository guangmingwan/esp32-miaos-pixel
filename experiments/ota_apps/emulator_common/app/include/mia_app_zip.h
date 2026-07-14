#pragma once

#include "mia_emulator_core.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

MiaCoreStatus mia_app_zip_extract(const char *path,
                                  const char *const *extensions, size_t extension_count,
                                  size_t max_size, uint8_t **out_data, size_t *out_size,
                                  char *out_name, size_t out_name_size);
MiaCoreStatus mia_app_zip_extract_into(const char *path,
                                       const char *const *extensions, size_t extension_count,
                                       uint8_t *output, size_t output_capacity, size_t *out_size,
                                       char *out_name, size_t out_name_size);

#ifdef __cplusplus
}
#endif
