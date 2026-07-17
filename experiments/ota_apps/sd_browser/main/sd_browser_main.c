/*
 * sd_browser_main — ported from experiments/elf_apps/sd_browser/src/main.c
 */

#include "mia_host_abi.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_FILES 96
#define VISIBLE_FILES 8
#define SCREEN_W 320
#define ROW_TOP 54
#define ROW_H 18
#define NAME_X 22
#define TYPE_X 138
#define SIZE_X 190
#define MOD_X 230

uint8_t sd_browser_host_language(void);
int32_t sd_browser_host_stat(const char *path, uint32_t *size, int64_t *modified);
int32_t sd_browser_host_capacity(uint64_t *free_bytes, uint64_t *total_bytes);

static MiaHostDirEntry files[MAX_FILES];
static uint32_t file_count;
static uint32_t selected_file;
static char current_path[1024] = "/";

static uint8_t delete_confirming;
static char delete_target[1280];
static uint8_t ui_language;

typedef struct {
  const char *title;
  const char *name;
  const char *type;
  const char *size;
  const char *modified;
  const char *folder;
  const char *file;
  const char *no_files;
  const char *free;
  const char *total;
  const char *controls;
  const char *delete_question;
  const char *delete_controls;
} BrowserText;

static const BrowserText TEXT_EN = {"SD Browser", "Name", "Type", "Size", "Modified",
                                    "Folder", "File", "No files", "Free", "Total",
                                    "UP/DN  A:Enter  B:Up  SEL+A:Del", "Delete?",
                                    "A:Delete  B:Cancel"};
static const BrowserText TEXT_ZH = {"文件浏览", "名称", "类型", "大小", "修改时间",
                                    "文件夹", "文件", "无文件", "可用", "共",
                                    "上/下  A:进入  B:返回  SEL+A:删除", "确认删除？",
                                    "A:删除  B:取消"};

static const BrowserText *ui_text(void) {
  return ui_language == 1 ? &TEXT_ZH : &TEXT_EN;
}

static uint8_t read_host_language(void) {
  return sd_browser_host_language();
}

static uint8_t exit_pressed(void) {
  return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
         mia_host_button_down(MIA_HOST_BUTTON_START);
}

static uint8_t delete_combination(void) {
  return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
         mia_host_button_pressed(MIA_HOST_BUTTON_A);
}

static void scan_current_directory(void) {
  int32_t result = mia_host_sd_list_dir(current_path, files, MAX_FILES);
  file_count = result > 0 ? (uint32_t)result : 0;
  selected_file = 0;
}

static void entry_path(uint32_t index, char *dest, uint32_t dest_size) {
  char base_path[sizeof(current_path)];
  if (file_count == 0 || index >= file_count) {
    strncpy(dest, current_path, dest_size - 1);
    dest[dest_size - 1] = 0;
    return;
  }
  strncpy(base_path, current_path, sizeof(base_path) - 1);
  base_path[sizeof(base_path) - 1] = 0;
  if (strcmp(base_path, "/") == 0) {
    snprintf(dest, dest_size, "/%s", files[index].name);
  } else {
    snprintf(dest, dest_size, "%s/%s", base_path, files[index].name);
  }
}

static void selected_entry_path(char *dest, uint32_t dest_size) {
  entry_path(selected_file, dest, dest_size);
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

static uint8_t utf8_len(const char *text) {
  unsigned char first = (unsigned char)text[0];
  size_t remaining = strnlen(text, 4);
  if (first < 0x80) {
    return 1;
  }
  if (remaining >= 2 && (first & 0xE0) == 0xC0 && (text[1] & 0xC0) == 0x80) {
    return 2;
  }
  if (remaining >= 3 && (first & 0xF0) == 0xE0 && (text[1] & 0xC0) == 0x80 &&
      (text[2] & 0xC0) == 0x80) {
    return 3;
  }
  if (remaining >= 4 && (first & 0xF8) == 0xF0 && (text[1] & 0xC0) == 0x80 &&
      (text[2] & 0xC0) == 0x80 && (text[3] & 0xC0) == 0x80) {
    return 4;
  }
  return 1;
}

static void copy_display_prefix(char *dest, uint32_t dest_size, const char *src,
                                uint32_t max_cells) {
  uint32_t used = 0;
  uint32_t out = 0;
  while (*src != 0 && out + 1 < dest_size) {
    uint8_t len = utf8_len(src);
    uint32_t cells = len == 1 ? 1 : 2;
    if (used + cells > max_cells || out + len >= dest_size) {
      break;
    }
    for (uint8_t i = 0; i < len; ++i) {
      dest[out++] = src[i];
    }
    src += len;
    used += cells;
  }
  if (*src != 0 && used < max_cells && out + 1 < dest_size) {
    dest[out++] = '.';
  }
  dest[out] = 0;
}

static void format_bytes(uint64_t bytes, char *dest, uint32_t dest_size) {
  static const char units[] = {'B', 'K', 'M', 'G'};
  uint32_t unit = 0;
  while (bytes >= 1024 && unit < 3) {
    uint64_t quotient = bytes / 1024;
    if (bytes % 1024 >= 512) {
      ++quotient;
    }
    bytes = quotient;
    ++unit;
  }
  if (dest_size == 0) {
    return;
  }

  char digits[21];
  uint32_t digit_count = 0;
  do {
    digits[digit_count++] = (char)('0' + bytes % 10);
    bytes /= 10;
  } while (bytes != 0 && digit_count < sizeof(digits));

  uint32_t output = 0;
  while (digit_count > 0 && output + 1 < dest_size) {
    dest[output++] = digits[--digit_count];
  }
  if (output + 1 < dest_size) {
    dest[output++] = units[unit];
  }
  dest[output] = 0;
}

static void format_date(int64_t modified, char *dest, uint32_t dest_size) {
  if (modified <= 0) {
    snprintf(dest, dest_size, "-");
    return;
  }
  time_t stamp = (time_t)modified;
  struct tm *info = localtime(&stamp);
  if (info == NULL || dest_size < 12) {
    snprintf(dest, dest_size, "-");
    return;
  }
  int month = info->tm_mon + 1;
  if (month < 1 || month > 12 || info->tm_mday < 1 || info->tm_mday > 31 ||
      info->tm_hour < 0 || info->tm_hour > 23 || info->tm_min < 0 || info->tm_min > 59) {
    snprintf(dest, dest_size, "-");
    return;
  }
  dest[0] = (char)('0' + month / 10);
  dest[1] = (char)('0' + month % 10);
  dest[2] = '-';
  dest[3] = (char)('0' + info->tm_mday / 10);
  dest[4] = (char)('0' + info->tm_mday % 10);
  dest[5] = ' ';
  dest[6] = (char)('0' + info->tm_hour / 10);
  dest[7] = (char)('0' + info->tm_hour % 10);
  dest[8] = ':';
  dest[9] = (char)('0' + info->tm_min / 10);
  dest[10] = (char)('0' + info->tm_min % 10);
  dest[11] = 0;
}

static void draw_folder_icon(int32_t x, int32_t y, uint8_t color) {
  mia_host_fill_rect(x + 1, y + 2, 5, 2, color);
  mia_host_fill_rect(x + 1, y + 4, 12, 1, color);
  mia_host_fill_rect(x, y + 5, 14, 8, color);
}

static void draw_file_icon(int32_t x, int32_t y, uint8_t color) {
  mia_host_fill_rect(x + 2, y + 1, 8, 1, color);
  mia_host_fill_rect(x + 2, y + 1, 1, 13, color);
  mia_host_fill_rect(x + 10, y + 3, 1, 11, color);
  mia_host_fill_rect(x + 2, y + 13, 9, 1, color);
  mia_host_fill_rect(x + 8, y + 1, 3, 3, color);
}

static void draw_table_header(const BrowserText *text) {
  mia_host_fill_rect(0, 37, SCREEN_W, 17, MIA_HOST_BLUE);
  mia_host_draw_text(NAME_X, 37, text->name, MIA_HOST_YELLOW, MIA_HOST_BLUE);
  mia_host_draw_text(TYPE_X, 37, text->type, MIA_HOST_YELLOW, MIA_HOST_BLUE);
  mia_host_draw_text(SIZE_X, 37, text->size, MIA_HOST_YELLOW, MIA_HOST_BLUE);
  mia_host_draw_text(MOD_X, 37, text->modified, MIA_HOST_YELLOW, MIA_HOST_BLUE);
}

static void draw_capacity_footer(const BrowserText *text) {
  uint64_t free_bytes = 0;
  uint64_t total_bytes = 0;
  char free_text[12];
  char total_text[12];
  char line[64];
  if (sd_browser_host_capacity(&free_bytes, &total_bytes) == 0) {
    format_bytes(free_bytes, free_text, sizeof(free_text));
    format_bytes(total_bytes, total_text, sizeof(total_text));
    snprintf(line, sizeof(line), "%s %s / %s %s", text->free, free_text, text->total,
             total_text);
  } else {
    snprintf(line, sizeof(line), "%s - / %s -", text->free, text->total);
  }
  mia_host_fill_rect(0, 198, SCREEN_W, 42, MIA_HOST_BLACK);
  mia_host_draw_text(8, 199, line, MIA_HOST_CYAN, MIA_HOST_BLACK);
  mia_host_draw_text(8, 220, text->controls, MIA_HOST_GRAY, MIA_HOST_BLACK);
}

static void draw_delete_dialog(const BrowserText *text) {
  mia_host_fill_rect(20, 80, 280, 70, MIA_HOST_BLUE);
  mia_host_fill_rect(22, 82, 276, 66, MIA_HOST_BLACK);
  mia_host_draw_text(40, 92, text->delete_question, MIA_HOST_RED, MIA_HOST_BLACK);
  char name_part[48];
  const char *slash = strrchr(delete_target, '/');
  if (slash) {
    copy_display_prefix(name_part, sizeof(name_part), slash + 1, 28);
  } else {
    copy_display_prefix(name_part, sizeof(name_part), delete_target, 28);
  }
  mia_host_draw_text(40, 112, name_part, MIA_HOST_WHITE, MIA_HOST_BLACK);
  mia_host_draw_text(40, 132, text->delete_controls, MIA_HOST_GRAY, MIA_HOST_BLACK);
}

static void draw_sd_browser(void) {
  const BrowserText *text = ui_text();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->title, MIA_HOST_BLACK,
                     MIA_HOST_YELLOW);
  char path_text[48];
  copy_display_prefix(path_text, sizeof(path_text), current_path, 38);
  mia_host_draw_text(4, 20, path_text, MIA_HOST_CYAN, MIA_HOST_BLACK);
  draw_table_header(text);

  if (file_count == 0) {
    mia_host_draw_text(118, 112, text->no_files, MIA_HOST_YELLOW, MIA_HOST_BLACK);
  }

  uint32_t first_visible = 0;
  if (selected_file >= VISIBLE_FILES) {
    first_visible = selected_file - VISIBLE_FILES + 1;
  }
  uint32_t visible_end = file_count < first_visible + VISIBLE_FILES
                             ? file_count
                             : first_visible + VISIBLE_FILES;
  for (uint32_t i = first_visible; i < visible_end; ++i) {
    int32_t y = ROW_TOP + (int32_t)(i - first_visible) * ROW_H;
    uint8_t selected = i == selected_file;
    uint8_t bg = selected ? MIA_HOST_BLUE : MIA_HOST_BLACK;
    uint8_t fg = selected ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
    char name_text[32];
    char size_text[12] = "-";
    char date_text[16] = "-";
    char path[sizeof(delete_target)];
    uint32_t size = 0;
    int64_t modified = 0;
    entry_path(i, path, sizeof(path));
    if (sd_browser_host_stat(path, &size, &modified) == 0) {
      if (!files[i].is_dir) {
        format_bytes(size, size_text, sizeof(size_text));
      }
      format_date(modified, date_text, sizeof(date_text));
    }
    copy_display_prefix(name_text, sizeof(name_text), files[i].name, 13);
    mia_host_fill_rect(2, y, 316, ROW_H - 1, bg);
    if (selected) {
      mia_host_fill_rect(2, y, 3, ROW_H - 1, MIA_HOST_YELLOW);
    }
    if (files[i].is_dir) {
      draw_folder_icon(6, y + 3, fg);
    } else {
      draw_file_icon(6, y + 2, fg);
    }
    mia_host_draw_text(NAME_X, y + 1, name_text, fg, bg);
    mia_host_draw_text(TYPE_X, y + 1, files[i].is_dir ? text->folder : text->file, fg, bg);
    mia_host_draw_text(SIZE_X, y + 1, size_text, fg, bg);
    mia_host_draw_text(MOD_X, y + 1, date_text, fg, bg);
  }

  draw_capacity_footer(text);
  if (delete_confirming) {
    draw_delete_dialog(text);
  }
  mia_host_present();
}

int sd_browser_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (mia_host_abi_version() != 2) {
    return 1;
  }
  strcpy(current_path, "/");
  ui_language = read_host_language();
  delete_confirming = 0;
  scan_current_directory();
  draw_sd_browser();
  while (1) {
    mia_host_buttons_poll();
    if (exit_pressed()) {
      break;
    }
    uint8_t changed = 0;

    if (delete_confirming) {
      if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
        mia_host_sd_remove(delete_target);
        delete_confirming = 0;
        scan_current_directory();
        changed = 1;
      } else if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
        delete_confirming = 0;
        changed = 1;
      }
    } else {
      if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && selected_file > 0) {
        --selected_file;
        changed = 1;
      }
      if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) && selected_file + 1 < file_count) {
        ++selected_file;
        changed = 1;
      }
      if (mia_host_button_pressed(MIA_HOST_BUTTON_A) &&
          !mia_host_button_down(MIA_HOST_BUTTON_SELECT)) {
        if (file_count > 0 && selected_file < file_count && files[selected_file].is_dir) {
          selected_entry_path(current_path, sizeof(current_path));
          scan_current_directory();
        }
        changed = 1;
      }
      if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
        navigate_to_parent();
        scan_current_directory();
        changed = 1;
      }
      if (delete_combination() && file_count > 0 && selected_file < file_count) {
        selected_entry_path(delete_target, sizeof(delete_target));
        delete_confirming = 1;
        changed = 1;
      }
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
