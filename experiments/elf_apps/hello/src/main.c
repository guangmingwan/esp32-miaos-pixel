#include "mia_host_abi.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (mia_host_abi_version() != 1) {
    mia_host_log("hello.app: unsupported MiaOS host ABI");
    return 1;
  }

  mia_host_log("hello.app: running interactive SD ELF");

  mia_host_clear(MIA_HOST_BLACK);
  mia_host_draw_text(72, 96, "Hello from SD ELF", MIA_HOST_WHITE, MIA_HOST_BLACK);
  mia_host_draw_text(60, 116, "SEL+ST: Exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
  mia_host_present();

  while (1) {
    const uint8_t select_down = mia_host_button_down(MIA_HOST_BUTTON_SELECT);
    const uint8_t start_down = mia_host_button_down(MIA_HOST_BUTTON_START);
    if (select_down && start_down) {
      break;
    }
    mia_host_delay_ms(20);
  }

  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  mia_host_log("hello.app: exit by SEL+ST");
  return 0;
}
