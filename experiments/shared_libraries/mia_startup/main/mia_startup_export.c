#include "mia_startup_menu_api.h"

static const MiaStartupMenuApi MIA_STARTUP_API = {
    .abi_version = MIA_STARTUP_MENU_ABI_VERSION,
    .struct_size = sizeof(MiaStartupMenuApi),
    .run_if_requested = mia_startup_menu_run_if_requested,
};

__attribute__((visibility("default")))
const MiaStartupMenuApi *mia_startup_menu_get_api(uint32_t requested_version) {
  return requested_version == MIA_STARTUP_MENU_ABI_VERSION ? &MIA_STARTUP_API : 0;
}
