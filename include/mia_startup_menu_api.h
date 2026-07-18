#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIA_STARTUP_MENU_ABI_VERSION 1u
#define MIA_STARTUP_MENU_LIBRARY_NAME "libmia_startup_v1.so"
#define MIA_STARTUP_MENU_LIBRARY_PATH "/sd/MiaOS/Library/libmia_startup_v1.so"

enum {
  MIA_STARTUP_BUTTON_START = 1,
  MIA_STARTUP_BUTTON_M = 2,
  MIA_STARTUP_BUTTON_SELECT = 5,
  MIA_STARTUP_BUTTON_UP = 10,
  MIA_STARTUP_BUTTON_DOWN = 11,
};

typedef struct {
  void (*poll_buttons)(void *context);
  uint8_t (*button_down)(uint8_t button, void *context);
  void (*render)(uint8_t selected, const char *status, void *context);
  void (*delay_ms)(uint32_t milliseconds, void *context);
  int32_t (*clear_nvs)(void *context);
  int32_t (*set_boot_slot)(uint8_t slot, void *context);
  void (*restart)(void *context);
  void *context;
} MiaStartupMenuHost;

typedef struct {
  uint32_t abi_version;
  uint32_t struct_size;
  int32_t (*run_if_requested)(const MiaStartupMenuHost *host);
} MiaStartupMenuApi;

typedef const MiaStartupMenuApi *(*MiaStartupMenuGetApiFn)(uint32_t requested_version);

const char *mia_startup_menu_item_label(uint8_t index);
int32_t mia_startup_menu_run_if_requested(const MiaStartupMenuHost *host);

#ifdef __cplusplus
}
#endif
