#include "mia_host_abi.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_FILES 96
#define VISIBLE_FILES 10

static MiaHostDirEntry files[MAX_FILES];
static uint32_t file_count;
static uint32_t selected_file;
static char current_path[128] = "/";
static uint8_t was_down[14];

static uint8_t pressed(uint8_t button) {
  uint8_t down = mia_host_button_down(button);
  uint8_t edge = down && !was_down[button];
  was_down[button] = down;
  return edge;
}

static uint8_t exit_pressed(void) {
  return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
         mia_host_button_down(MIA_HOST_BUTTON_START);
}

static void scan_current_directory(void) {
  int32_t result = mia_host_sd_list_dir(current_path, files, MAX_FILES);
  file_count = result > 0 ? (uint32_t)result : 0;
  selected_file = 0;
}

static void selected_entry_path(char *dest, uint32_t dest_size) {
  if (file_count == 0 || selected_file >= file_count) {
    strncpy(dest, current_path, dest_size - 1);
    dest[dest_size - 1] = 0;
    return;
  }
  if (strcmp(current_path, "/") == 0) {
    snprintf(dest, dest_size, "/%s", files[selected_file].name);
  } else {
    snprintf(dest, dest_size, "%s/%s", current_path, files[selected_file].name);
  }
}

static void navigate_to_parent(void) {
  if (strcmp(current_path, "/") == 0) {
    return;
  }
  char *slash = strrchr(current_path, '/');
  if (slash == NULL || slash == current_path) {
    strcpy(current_path, "/");
  } else {
    *slash = 0;
  }
}

static void draw_sd_browser(void) {
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, "SD Browser", MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 22, current_path, MIA_HOST_CYAN, MIA_HOST_BLACK);

  if (file_count == 0) {
    mia_host_draw_text(128, 92, "No files", MIA_HOST_YELLOW, MIA_HOST_BLACK);
  }

  uint32_t first_visible = 0;
  if (selected_file >= VISIBLE_FILES) {
    first_visible = selected_file - VISIBLE_FILES + 1;
  }
  uint32_t visible_end = file_count < first_visible + VISIBLE_FILES
                             ? file_count
                             : first_visible + VISIBLE_FILES;
  for (uint32_t i = first_visible; i < visible_end; ++i) {
    int32_t y = 42 + (int32_t)(i - first_visible) * 18;
    uint8_t selected = i == selected_file;
    uint8_t bg = selected ? MIA_HOST_BLUE : MIA_HOST_BLACK;
    uint8_t fg = selected ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
    char line[76];
    snprintf(line, sizeof(line), files[i].is_dir ? "%s/" : "%s", files[i].name);
    mia_host_fill_rect(4, y - 2, 312, 14, bg);
    mia_host_draw_text(8, y, line, fg, bg);
  }

  mia_host_draw_text(8, 222, "UP/DN Scroll  A:Enter  B:Up", MIA_HOST_GRAY,
                     MIA_HOST_BLACK);
  mia_host_present();
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (mia_host_abi_version() != 1) {
    return 1;
  }
  strcpy(current_path, "/");
  scan_current_directory();
  draw_sd_browser();
  while (!exit_pressed()) {
    uint8_t changed = 0;
    if (pressed(MIA_HOST_BUTTON_UP) && selected_file > 0) {
      --selected_file;
      changed = 1;
    }
    if (pressed(MIA_HOST_BUTTON_DOWN) && selected_file + 1 < file_count) {
      ++selected_file;
      changed = 1;
    }
    if (pressed(MIA_HOST_BUTTON_A)) {
      if (file_count > 0 && selected_file < file_count && files[selected_file].is_dir) {
        selected_entry_path(current_path, sizeof(current_path));
        scan_current_directory();
      }
      changed = 1;
    }
    if (pressed(MIA_HOST_BUTTON_B)) {
      navigate_to_parent();
      scan_current_directory();
      changed = 1;
    }
    if (changed) {
      draw_sd_browser();
    }
    mia_host_delay_ms(20);
  }
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  return 0;
}
