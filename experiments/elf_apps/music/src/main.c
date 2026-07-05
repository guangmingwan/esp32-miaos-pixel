#include "mia_host_abi.h"
#include "music_player.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MUSIC_MAX_ENTRIES 48
#define MUSIC_VISIBLE_ENTRIES 9

static MiaHostDirEntry entries[MUSIC_MAX_ENTRIES];
static uint32_t entry_count;
static uint32_t selected_entry;
static char current_path[128] = "/";
static char status_text[48] = "A:Open  B:Up  SEL+ST:Exit";

static const char *const MUSIC_START_PATHS[] = {
    "/MiaOS/Media",
    "/Media",
    "/",
};

static uint8_t exit_pressed(void) {
  return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
         mia_host_button_down(MIA_HOST_BUTTON_START);
}

static void copy_text(char *dest, size_t dest_size, const char *value) {
  if (dest == NULL || dest_size == 0) {
    return;
  }
  if (value == NULL) {
    dest[0] = '\0';
    return;
  }
  snprintf(dest, dest_size, "%s", value);
}

static int is_directory(const MiaHostDirEntry *entry) {
  return entry != NULL && entry->is_dir != 0;
}

static void build_child_path(char *dest, uint32_t dest_size, const char *base,
                             const char *name) {
  if (strcmp(base, "/") == 0) {
    snprintf(dest, dest_size, "/%s", name);
  } else {
    snprintf(dest, dest_size, "%s/%s", base, name);
  }
}

static void build_vfs_path(char *dest, uint32_t dest_size, const char *base,
                           const char *name) {
  if (strcmp(base, "/") == 0) {
    snprintf(dest, dest_size, "/sd/%s", name);
  } else {
    snprintf(dest, dest_size, "/sd%s/%s", base, name);
  }
}

static void scan_current_directory(void) {
  entry_count = 0;
  selected_entry = 0;
  const int32_t result = mia_host_sd_list_dir(current_path, entries, MUSIC_MAX_ENTRIES);
  if (result <= 0) {
    copy_text(status_text, sizeof(status_text), "Directory empty");
    return;
  }

  uint32_t count = 0;
  for (int32_t index = 0; index < result && count < MUSIC_MAX_ENTRIES; ++index) {
    if (is_directory(&entries[index]) || music_is_supported_file(entries[index].name)) {
      if ((uint32_t)index != count) {
        entries[count] = entries[index];
      }
      ++count;
    }
  }
  entry_count = count;
  copy_text(status_text, sizeof(status_text),
            count == 0 ? "No audio files" : "A:Open  B:Up  SEL+ST:Exit");
}

static void choose_initial_directory(void) {
  for (uint32_t index = 0;
       index < sizeof(MUSIC_START_PATHS) / sizeof(MUSIC_START_PATHS[0]);
       ++index) {
    copy_text(current_path, sizeof(current_path), MUSIC_START_PATHS[index]);
    scan_current_directory();
    if (entry_count > 0 || strcmp(current_path, "/") == 0) {
      return;
    }
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
    *slash = '\0';
  }
}

static void draw_music_app(void) {
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, "Music", MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 24, current_path, MIA_HOST_CYAN, MIA_HOST_BLACK);

  if (entry_count == 0) {
    mia_host_draw_text(84, 96, "No audio files", MIA_HOST_RED, MIA_HOST_BLACK);
  }

  uint32_t first_visible = 0;
  if (selected_entry >= MUSIC_VISIBLE_ENTRIES) {
    first_visible = selected_entry - MUSIC_VISIBLE_ENTRIES + 1;
  }
  uint32_t visible_end = entry_count < first_visible + MUSIC_VISIBLE_ENTRIES
                             ? entry_count
                             : first_visible + MUSIC_VISIBLE_ENTRIES;
  for (uint32_t index = first_visible; index < visible_end; ++index) {
    int32_t y = 48 + (int32_t)(index - first_visible) * 18;
    uint8_t bg = index == selected_entry ? MIA_HOST_BLUE : MIA_HOST_BLACK;
    uint8_t fg = index == selected_entry ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
    char line[72];
    if (entries[index].is_dir) {
      snprintf(line, sizeof(line), "%s/", entries[index].name);
    } else {
      snprintf(line, sizeof(line), "%s", entries[index].name);
    }
    mia_host_fill_rect(4, y - 2, 312, 14, bg);
    mia_host_draw_text(8, y, line, fg, bg);
  }

  mia_host_draw_text(8, 204, status_text, MIA_HOST_GREEN, MIA_HOST_BLACK);
  mia_host_draw_text(8, 222, "UP/DN Move  A:Open  B:Up", MIA_HOST_GRAY,
                     MIA_HOST_BLACK);
  mia_host_present();
}

static void open_selected_entry(void) {
  if (entry_count == 0 || selected_entry >= entry_count) {
    return;
  }
  if (entries[selected_entry].is_dir) {
    build_child_path(current_path, sizeof(current_path), current_path,
                     entries[selected_entry].name);
    scan_current_directory();
    return;
  }

  char file_path[192];
  build_vfs_path(file_path, sizeof(file_path), current_path,
                 entries[selected_entry].name);
  music_play_file(file_path, status_text, sizeof(status_text));
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (mia_host_abi_version() != 2) {
    return 1;
  }

  choose_initial_directory();
  draw_music_app();
  while (1) {
    mia_host_buttons_poll();
    if (exit_pressed()) {
      break;
    }
    uint8_t changed = 0;
    if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && selected_entry > 0) {
      --selected_entry;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) &&
        selected_entry + 1 < entry_count) {
      ++selected_entry;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      open_selected_entry();
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
      navigate_to_parent();
      scan_current_directory();
      changed = 1;
    }
    if (changed) {
      draw_music_app();
    }
    mia_host_delay_ms(20);
  }

  mia_host_audio_close();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  return 0;
}
