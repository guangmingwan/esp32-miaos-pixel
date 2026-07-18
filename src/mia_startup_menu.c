#include "mia_startup_menu_api.h"

#include <stddef.h>

static const char *const menu_items[] = {
    "Clear NVRAM",
    "Boot ota_0",
    "Boot ota_1",
};

const char *mia_startup_menu_item_label(uint8_t index) {
  return index < (uint8_t)(sizeof(menu_items) / sizeof(menu_items[0])) ? menu_items[index] : "";
}

int32_t mia_startup_menu_run_if_requested(const MiaStartupMenuHost *host) {
  if (host == NULL || host->poll_buttons == NULL || host->button_down == NULL ||
      host->render == NULL || host->delay_ms == NULL || host->clear_nvs == NULL ||
      host->set_boot_slot == NULL || host->restart == NULL) {
    return 0;
  }

  host->poll_buttons(host->context);
  if (!host->button_down(MIA_STARTUP_BUTTON_M, host->context)) return 0;

  uint8_t selected = 0;
  host->render(selected, "M held: release to continue", host->context);
  while (host->button_down(MIA_STARTUP_BUTTON_M, host->context)) {
    host->poll_buttons(host->context);
    host->delay_ms(10, host->context);
  }

  host->render(selected, "UP/DOWN: select  START: confirm  SELECT: cancel", host->context);
  uint8_t previous_up = 0;
  uint8_t previous_down = 0;
  uint8_t previous_start = 0;
  uint8_t previous_select = 0;
  for (;;) {
    host->poll_buttons(host->context);
    const uint8_t up = host->button_down(MIA_STARTUP_BUTTON_UP, host->context);
    const uint8_t down = host->button_down(MIA_STARTUP_BUTTON_DOWN, host->context);
    const uint8_t start = host->button_down(MIA_STARTUP_BUTTON_START, host->context);
    const uint8_t cancel = host->button_down(MIA_STARTUP_BUTTON_SELECT, host->context);

    if (up && !previous_up) {
      selected = selected == 0 ? 2 : selected - 1;
      host->render(selected, "UP/DOWN: select  START: confirm  SELECT: cancel", host->context);
    }
    if (down && !previous_down) {
      selected = selected == 2 ? 0 : selected + 1;
      host->render(selected, "UP/DOWN: select  START: confirm  SELECT: cancel", host->context);
    }
    if (cancel && !previous_select) return 1;

    if (start && !previous_start) {
      int32_t result = selected == 0
                           ? host->clear_nvs(host->context)
                           : host->set_boot_slot((uint8_t)(selected - 1), host->context);
      if (result == 0 && selected == 0) {
        result = host->set_boot_slot(0, host->context);
      }
      if (result != 0) {
        host->render(selected, "Operation failed; SELECT: cancel", host->context);
      } else {
        host->render(selected, "Restarting...", host->context);
        host->delay_ms(100, host->context);
        host->restart(host->context);
        return 2;
      }
    }

    previous_up = up;
    previous_down = down;
    previous_start = start;
    previous_select = cancel;
    host->delay_ms(20, host->context);
  }
}
