#include <stdint.h>

uint32_t mia_host_abi_version(void);
void mia_host_log(const char *message);
int32_t mia_host_screen_width(void);
int32_t mia_host_screen_height(void);
void mia_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color);
void mia_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg,
                        uint8_t bg);
void mia_host_present(void);

enum MiaPalette {
  MIA_BLACK = 0,
  MIA_WHITE = 1,
  MIA_BLUE = 2,
  MIA_GREEN = 3,
  MIA_RED = 4,
  MIA_YELLOW = 5,
  MIA_CYAN = 6,
  MIA_GRAY = 7,
  MIA_DARK_BLUE = 8,
};

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  mia_host_log("mia_test.app: start");

  if (mia_host_abi_version() != 1) {
    mia_host_log("mia_test.app: unsupported MiaOS host ABI");
    return 1;
  }

  mia_host_log("mia_test.app: ABI v1 OK");
  const int32_t screen_w = mia_host_screen_width();
  const int32_t screen_h = mia_host_screen_height();

  mia_host_fill_rect(0, 0, screen_w, screen_h, MIA_DARK_BLUE);
  mia_host_fill_rect(0, 0, screen_w, 28, MIA_YELLOW);
  mia_host_fill_rect(24, 56, screen_w - 48, 96, MIA_BLUE);
  mia_host_fill_rect(34, 66, screen_w - 68, 76, MIA_CYAN);
  mia_host_draw_text(8, 10, "MiaOS ELF Test", MIA_BLACK, MIA_YELLOW);
  mia_host_draw_text(54, 86, "SD app drawing OK", MIA_BLACK, MIA_CYAN);
  mia_host_draw_text(54, 106, "ABI v1 visual path", MIA_BLACK, MIA_CYAN);
  mia_host_draw_text(64, screen_h - 26, "/MiaOS/Application/mia_test.app", MIA_WHITE,
                     MIA_DARK_BLUE);
  mia_host_present();
  mia_host_log("mia_test.app: drew visual test screen");
  return 0;
}
