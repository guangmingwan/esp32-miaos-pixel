#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIA_TEXT_ABI_VERSION 1u
#define MIA_TEXT_LIBRARY_NAME "libmia_text_v1.so"
#define MIA_TEXT_LIBRARY_PATH "/sd/MiaOS/Library/libmia_text_v1.so"

typedef struct MiaTextApi {
  uint32_t abi_version;
  uint32_t struct_size;
  int32_t (*draw_text)(uint8_t *pixels, int32_t surface_width,
                       int32_t surface_height, int32_t x, int32_t y,
                       const char *text, uint8_t fg, uint8_t bg);
} MiaTextApi;

typedef const MiaTextApi *(*MiaTextGetApiFn)(uint32_t requested_version);

const MiaTextApi *mia_text_get_api(uint32_t requested_version);

#ifdef __cplusplus
}
#endif
