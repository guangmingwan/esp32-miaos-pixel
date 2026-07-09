#include "int_wdt_guard.h"
#include "mia_host_abi.h"

#include <esp_heap_caps.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace {

enum MiaPalette : uint8_t {
  MIA_BLACK = 0,
  MIA_WHITE = 1,
  MIA_BLUE = 2,
  MIA_GREEN = 3,
  MIA_RED = 4,
  MIA_YELLOW = 5,
  MIA_CYAN = 6,
  MIA_GRAY = 7,
};

constexpr size_t TEST_STEP = 4096;
constexpr size_t INTERNAL_SCRATCH_SIZE = 4096;
constexpr size_t MIN_TEST_SIZE = 64 * 1024;
constexpr size_t SKIP_INTERVAL = 0x4000;

char g_status[64] = "Press A to run max PSRAM test";
bool g_running = false;
uint32_t g_totalKb = 0;

bool exit_pressed() {
  return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
         mia_host_button_down(MIA_HOST_BUTTON_START);
}

void draw_psram_test(const char *status, uint32_t testedKb, uint32_t totalKb, bool done,
                     bool ok) {
  mia_host_clear(MIA_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_YELLOW);
  mia_host_draw_text(4, 6, "PSRAM Test", MIA_BLACK, MIA_YELLOW);

  char line[48];
  snprintf(line, sizeof(line), "Total %luK Free %luK",
           static_cast<unsigned long>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024),
           static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
  mia_host_draw_text(8, 36, line, MIA_CYAN, MIA_BLACK);
  snprintf(line, sizeof(line), "Max block %luK",
           static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024));
  mia_host_draw_text(8, 54, line, MIA_CYAN, MIA_BLACK);
  snprintf(line, sizeof(line), "Tested %luK / %luK", static_cast<unsigned long>(testedKb),
           static_cast<unsigned long>(totalKb));
  mia_host_draw_text(8, 82, line, done ? (ok ? MIA_GREEN : MIA_RED) : MIA_YELLOW,
                     MIA_BLACK);
  mia_host_draw_text(8, 106, status, done ? (ok ? MIA_GREEN : MIA_RED) : MIA_WHITE,
                     MIA_BLACK);
  mia_host_draw_text(8, 222, "A:Run max test  SEL+ST:Exit", MIA_GRAY, MIA_BLACK);
  mia_host_present();
}

uint8_t expected_pattern(size_t index, uint8_t salt) {
  return static_cast<uint8_t>((index * 33U) ^ (index >> 3U) ^ salt);
}

bool allocate_max_psram(uint8_t **buffer, size_t *size) {
  size_t candidate = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) & ~(TEST_STEP - 1U);
  while (candidate >= MIN_TEST_SIZE) {
    uint8_t *ptr = static_cast<uint8_t *>(
        heap_caps_malloc(candidate, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (ptr != nullptr) {
      *buffer = ptr;
      *size = candidate;
      return true;
    }
    candidate -= TEST_STEP;
  }
  return false;
}

bool should_skip_offset(size_t offset) {
  return offset != 0 && (offset % SKIP_INTERVAL) == 0;
}

void update_progress(const char *phase, size_t offset, size_t total) {
  snprintf(g_status, sizeof(g_status), "%s 0x%08x", phase, static_cast<unsigned>(offset));
  draw_psram_test(g_status, static_cast<uint32_t>(offset / 1024), g_totalKb, false, false);
  mia_host_delay_ms(1);
}

bool fill_and_verify(uint8_t *buffer, size_t size, uint8_t salt) {
  for (size_t offset = 0; offset < size; offset += TEST_STEP) {
    if (should_skip_offset(offset)) {
      continue;
    }
    update_progress("fill", offset, size);
    const size_t chunk = size - offset < TEST_STEP ? size - offset : TEST_STEP;
    for (size_t i = 0; i < chunk; ++i) {
      buffer[offset + i] = expected_pattern(offset + i, salt);
    }
  }

  for (size_t offset = 0; offset < size; offset += TEST_STEP) {
    if (should_skip_offset(offset)) {
      continue;
    }
    update_progress("verify", offset, size);
    const size_t chunk = size - offset < TEST_STEP ? size - offset : TEST_STEP;
    for (size_t i = 0; i < chunk; ++i) {
      const size_t index = offset + i;
      if (buffer[index] != expected_pattern(index, salt)) {
        return false;
      }
    }
  }
  return true;
}

bool copy_from_internal_and_verify(uint8_t *buffer, size_t size) {
  uint8_t *scratch = static_cast<uint8_t *>(
      heap_caps_malloc(INTERNAL_SCRATCH_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (scratch == nullptr) {
    return false;
  }

  for (size_t offset = 0; offset < size; offset += INTERNAL_SCRATCH_SIZE) {
    if (should_skip_offset(offset)) {
      continue;
    }
    update_progress("copy-int", offset, size);
    const size_t chunk = size - offset < INTERNAL_SCRATCH_SIZE ? size - offset : INTERNAL_SCRATCH_SIZE;
    for (size_t i = 0; i < chunk; ++i) {
      scratch[i] = expected_pattern(offset + i, 0x5a);
    }
    memcpy(buffer + offset, scratch, chunk);
  }

  heap_caps_free(scratch);
  for (size_t offset = 0; offset < size; offset += TEST_STEP) {
    if (should_skip_offset(offset)) {
      continue;
    }
    update_progress("verify-int", offset, size);
    const size_t chunk = size - offset < TEST_STEP ? size - offset : TEST_STEP;
    for (size_t i = 0; i < chunk; ++i) {
      const size_t index = offset + i;
      if (buffer[index] != expected_pattern(index, 0x5a)) {
        return false;
      }
    }
  }
  return true;
}

bool copy_within_psram_and_verify(uint8_t *buffer, size_t size) {
  const size_t half = (size / 2U) & ~(TEST_STEP - 1U);
  if (half < MIN_TEST_SIZE) {
    return true;
  }

  for (size_t offset = 0; offset < half; offset += TEST_STEP) {
    if (should_skip_offset(offset) || should_skip_offset(half + offset)) {
      continue;
    }
    update_progress("copy-psram", offset, half);
    memcpy(buffer + half + offset, buffer + offset, TEST_STEP);
  }

  for (size_t offset = 0; offset < half; offset += TEST_STEP) {
    if (should_skip_offset(offset) || should_skip_offset(half + offset)) {
      continue;
    }
    update_progress("verify-psram", offset, half);
    if (memcmp(buffer + offset, buffer + half + offset, TEST_STEP) != 0) {
      return false;
    }
  }
  return true;
}

void run_psram_test() {
  if (g_running) {
    return;
  }
  g_running = true;

  uint8_t *buffer = nullptr;
  size_t size = 0;
  if (!allocate_max_psram(&buffer, &size)) {
    snprintf(g_status, sizeof(g_status), "PSRAM alloc failed");
    draw_psram_test(g_status, 0, 0, true, false);
    g_running = false;
    return;
  }

  g_totalKb = static_cast<uint32_t>(size / 1024);
  snprintf(g_status, sizeof(g_status), "Running serial-only test");
  draw_psram_test(g_status, 0, g_totalKb, false, false);
  ScopedIntWdtPause wdtGuard;
  const bool ok = fill_and_verify(buffer, size, 0xa5) &&
                  copy_from_internal_and_verify(buffer, size) &&
                  fill_and_verify(buffer, size, 0x3c) &&
                  copy_within_psram_and_verify(buffer, size);
  heap_caps_free(buffer);
  snprintf(g_status, sizeof(g_status), ok ? "PASS max PSRAM block" : "FAIL see serial log");
  draw_psram_test(g_status, size / 1024, size / 1024, true, ok);
  g_running = false;
}

}

extern "C" int psram_test_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (mia_host_abi_version() != 2) {
    return 1;
  }

  snprintf(g_status, sizeof(g_status), "Press A to run max PSRAM test");
  draw_psram_test(g_status, 0, 0, false, false);

  while (1) {
    mia_host_buttons_poll();
    if (exit_pressed()) {
      break;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      run_psram_test();
    }
    mia_host_delay_ms(20);
  }

  mia_host_clear(MIA_BLACK);
  mia_host_present();
  return 0;
}
