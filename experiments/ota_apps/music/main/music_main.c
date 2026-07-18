#include "mia_host_abi.h"
#include "launch_context.h"
#include "music_i18n.h"
#include "music_player.h"

#include <esp_log.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MUSIC_MAX_ENTRIES 48
#define MUSIC_VISIBLE_ENTRIES 8

#ifndef MIA_MUSIC_AUTOPLAY_FIRST
#define MIA_MUSIC_AUTOPLAY_FIRST 0
#endif

static MiaHostDirEntry entries[MUSIC_MAX_ENTRIES];
static uint32_t entry_count;
static uint32_t selected_entry;
static char current_path[128] = "/music";
static char status_text[48];
static uint8_t autoplay_attempted;

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

static void build_audio_path(char *dest, uint32_t dest_size, const char *base,
                             const char *name) {
  if (strcmp(base, "/") == 0) {
    snprintf(dest, dest_size, "/%s", name);
  } else {
    snprintf(dest, dest_size, "%s/%s", base, name);
  }
}

static void scan_current_directory(void) {
  const MusicText *text = music_text();
  entry_count = 0;
  selected_entry = 0;
  int32_t result = mia_host_sd_list_dir(current_path, entries, MUSIC_MAX_ENTRIES);
  if (result <= 0 && strcmp(current_path, "/music") == 0) {
    copy_text(current_path, sizeof(current_path), "/");
    result = mia_host_sd_list_dir(current_path, entries, MUSIC_MAX_ENTRIES);
  }
  if (result <= 0) {
    copy_text(status_text, sizeof(status_text), text->directory_empty);
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
            count == 0 ? text->no_audio_files : text->browse_status);
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
  const MusicText *text = music_text();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->title, MIA_HOST_BLACK,
                     MIA_HOST_YELLOW);
  mia_host_draw_text(4, 24, current_path, MIA_HOST_CYAN, MIA_HOST_BLACK);

  if (entry_count == 0) {
    mia_host_draw_text(84, 96, text->no_audio_files, MIA_HOST_RED, MIA_HOST_BLACK);
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
    mia_host_fill_rect(4, y - 2, 312, 16, bg);
    mia_host_draw_text(8, y, line, fg, bg);
  }

  mia_host_draw_text(8, 204, status_text, MIA_HOST_GREEN, MIA_HOST_BLACK);
  mia_host_draw_text(8, 222, text->browse_controls, MIA_HOST_GRAY,
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
  build_audio_path(file_path, sizeof(file_path), current_path,
                   entries[selected_entry].name);
  music_play_file(file_path, status_text, sizeof(status_text));
}

static void maybe_autoplay_first_file(void) {
#if MIA_MUSIC_AUTOPLAY_FIRST
  if (autoplay_attempted != 0) {
    return;
  }
  autoplay_attempted = 1;

  for (uint32_t index = 0; index < entry_count; ++index) {
    if (entries[index].is_dir) {
      continue;
    }

    char file_path[192];

    build_audio_path(file_path, sizeof(file_path), current_path, entries[index].name);
    music_play_file(file_path, status_text, sizeof(status_text));
    return;
  }
#endif
}

int music_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (mia_host_abi_version() != 2) {
    return 1;
  }

  scan_current_directory();
  char direct_path[MIA_HOST_LAUNCH_ARG_SIZE];
  if (mia_host_consume_launch_arg("music", direct_path, sizeof(direct_path))) {
    music_play_file(direct_path, status_text, sizeof(status_text));
  } else {
    maybe_autoplay_first_file();
  }
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
      /* re-poll to flush any stale button state accumulated during
       * the blocking playback call (e.g. B used to stop playback) */
      mia_host_buttons_poll();
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
