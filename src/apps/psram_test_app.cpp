#include "apps/psram_test_app.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <string.h>

#include "int_wdt_guard.h"
#include "lava_native_display.h"

namespace {

enum LavaPalette : uint8_t {
  LAVA_BLACK = 0,
  LAVA_WHITE = 1,
  LAVA_BLUE = 2,
  LAVA_GREEN = 3,
  LAVA_RED = 4,
  LAVA_YELLOW = 5,
  LAVA_CYAN = 6,
  LAVA_GRAY = 7,
};

constexpr size_t TEST_STEP = 4096;
constexpr size_t INTERNAL_SCRATCH_SIZE = 4096;
constexpr size_t MIN_TEST_SIZE = 64 * 1024;
constexpr size_t SKIP_INTERVAL = 0x4000;

char g_status[64] = "Press A to run max PSRAM test";
bool g_running = false;
uint32_t g_totalKb = 0;

void drawPsramTest(const char *status, uint32_t testedKb, uint32_t totalKb, bool done, bool ok) {
  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "PSRAM Test", LAVA_BLACK, LAVA_YELLOW);

  char line[48];
  snprintf(line, sizeof(line), "Total %luK Free %luK",
           static_cast<unsigned long>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024),
           static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
  lavaDrawText(8, 36, line, LAVA_CYAN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Max block %luK",
           static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024));
  lavaDrawText(8, 54, line, LAVA_CYAN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Tested %luK / %luK", static_cast<unsigned long>(testedKb),
           static_cast<unsigned long>(totalKb));
  lavaDrawText(8, 82, line, done ? (ok ? LAVA_GREEN : LAVA_RED) : LAVA_YELLOW, LAVA_BLACK);
  lavaDrawText(8, 106, status, done ? (ok ? LAVA_GREEN : LAVA_RED) : LAVA_WHITE, LAVA_BLACK);
  lavaDrawText(8, 222, "A:Run max test  B:Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

uint8_t expectedPattern(size_t index, uint8_t salt) {
  return static_cast<uint8_t>((index * 33U) ^ (index >> 3U) ^ salt);
}

bool allocateMaxPsram(uint8_t **buffer, size_t *size) {
  size_t candidate = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) & ~(TEST_STEP - 1U);
  while (candidate >= MIN_TEST_SIZE) {
    uint8_t *ptr = static_cast<uint8_t *>(heap_caps_malloc(candidate, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (ptr != nullptr) {
      *buffer = ptr;
      *size = candidate;
      return true;
    }
    candidate -= TEST_STEP;
  }
  return false;
}

bool shouldSkipOffset(size_t offset) {
  return offset != 0 && (offset % SKIP_INTERVAL) == 0;
}

void logSkip(const char *phase, size_t offset, size_t total) {
  Serial.printf("[psram-test] %s skip offset=0x%08x total=0x%08x\n", phase,
                static_cast<unsigned>(offset), static_cast<unsigned>(total));
  delay(1);
}

void updateProgress(const char *phase, size_t offset, size_t total) {
  Serial.printf("[psram-test] %s offset=0x%08x total=0x%08x\n", phase,
                static_cast<unsigned>(offset), static_cast<unsigned>(total));
  snprintf(g_status, sizeof(g_status), "%s 0x%08x", phase, static_cast<unsigned>(offset));
  drawPsramTest(g_status, static_cast<uint32_t>(offset / 1024), g_totalKb, false, false);
  delay(1);
}

bool fillAndVerify(uint8_t *buffer, size_t size, uint8_t salt) {
  Serial.printf("[psram-test] fill start ptr=%p size=0x%08x salt=0x%02x\n", buffer,
                static_cast<unsigned>(size), salt);
  for (size_t offset = 0; offset < size; offset += TEST_STEP) {
    if (shouldSkipOffset(offset)) {
      logSkip("fill", offset, size);
      continue;
    }
    updateProgress("fill", offset, size);
    const size_t chunk = min(TEST_STEP, size - offset);
    for (size_t i = 0; i < chunk; ++i) {
      buffer[offset + i] = expectedPattern(offset + i, salt);
    }
  }

  Serial.println("[psram-test] verify start");
  for (size_t offset = 0; offset < size; offset += TEST_STEP) {
    if (shouldSkipOffset(offset)) {
      logSkip("verify", offset, size);
      continue;
    }
    updateProgress("verify", offset, size);
    const size_t chunk = min(TEST_STEP, size - offset);
    for (size_t i = 0; i < chunk; ++i) {
      const size_t index = offset + i;
      const uint8_t expected = expectedPattern(index, salt);
      if (buffer[index] != expected) {
        Serial.printf("[psram-test] verify mismatch index=0x%08x got=0x%02x expected=0x%02x\n",
                      static_cast<unsigned>(index), buffer[index], expected);
        return false;
      }
    }
  }
  return true;
}

bool copyFromInternalAndVerify(uint8_t *buffer, size_t size) {
  uint8_t *scratch = static_cast<uint8_t *>(heap_caps_malloc(INTERNAL_SCRATCH_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (scratch == nullptr) {
    Serial.println("[psram-test] internal scratch alloc failed");
    return false;
  }

  Serial.println("[psram-test] internal to psram copy start");
  for (size_t offset = 0; offset < size; offset += INTERNAL_SCRATCH_SIZE) {
    if (shouldSkipOffset(offset)) {
      logSkip("copy-int", offset, size);
      continue;
    }
    updateProgress("copy-int", offset, size);
    const size_t chunk = min(INTERNAL_SCRATCH_SIZE, size - offset);
    for (size_t i = 0; i < chunk; ++i) {
      scratch[i] = expectedPattern(offset + i, 0x5a);
    }
    memcpy(buffer + offset, scratch, chunk);
  }

  heap_caps_free(scratch);
  Serial.println("[psram-test] internal copy verify start");
  for (size_t offset = 0; offset < size; offset += TEST_STEP) {
    if (shouldSkipOffset(offset)) {
      logSkip("verify-int", offset, size);
      continue;
    }
    updateProgress("verify-int", offset, size);
    const size_t chunk = min(TEST_STEP, size - offset);
    for (size_t i = 0; i < chunk; ++i) {
      const size_t index = offset + i;
      const uint8_t expected = expectedPattern(index, 0x5a);
      if (buffer[index] != expected) {
        Serial.printf("[psram-test] copy-int mismatch index=0x%08x got=0x%02x expected=0x%02x\n",
                      static_cast<unsigned>(index), buffer[index], expected);
        return false;
      }
    }
  }
  return true;
}

bool copyWithinPsramAndVerify(uint8_t *buffer, size_t size) {
  const size_t half = (size / 2U) & ~(TEST_STEP - 1U);
  if (half < MIN_TEST_SIZE) {
    Serial.println("[psram-test] psram copy skipped: block too small");
    return true;
  }

  Serial.printf("[psram-test] psram to psram copy start half=0x%08x\n", static_cast<unsigned>(half));
  for (size_t offset = 0; offset < half; offset += TEST_STEP) {
    if (shouldSkipOffset(offset) || shouldSkipOffset(half + offset)) {
      logSkip("copy-psram", offset, half);
      continue;
    }
    updateProgress("copy-psram", offset, half);
    memcpy(buffer + half + offset, buffer + offset, TEST_STEP);
  }

  Serial.println("[psram-test] psram copy verify start");
  for (size_t offset = 0; offset < half; offset += TEST_STEP) {
    if (shouldSkipOffset(offset) || shouldSkipOffset(half + offset)) {
      logSkip("verify-psram", offset, half);
      continue;
    }
    updateProgress("verify-psram", offset, half);
    if (memcmp(buffer + offset, buffer + half + offset, TEST_STEP) != 0) {
      Serial.printf("[psram-test] psram copy mismatch offset=0x%08x\n", static_cast<unsigned>(offset));
      return false;
    }
  }
  return true;
}

void runPsramTest() {
  if (g_running) {
    return;
  }
  g_running = true;

  uint8_t *buffer = nullptr;
  size_t size = 0;
  Serial.printf("[psram-test] heap total=%u free=%u largest=%u\n",
                static_cast<unsigned>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM)),
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
                static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)));
  if (!allocateMaxPsram(&buffer, &size)) {
    snprintf(g_status, sizeof(g_status), "PSRAM alloc failed");
    Serial.println("[psram-test] alloc failed");
    drawPsramTest(g_status, 0, 0, true, false);
    g_running = false;
    return;
  }

  Serial.printf("[psram-test] alloc ok ptr=%p size=0x%08x\n", buffer, static_cast<unsigned>(size));
  g_totalKb = static_cast<uint32_t>(size / 1024);
  snprintf(g_status, sizeof(g_status), "Running serial-only test");
  drawPsramTest(g_status, 0, g_totalKb, false, false);
  ScopedIntWdtPause wdtGuard;
  const bool ok = fillAndVerify(buffer, size, 0xa5) && copyFromInternalAndVerify(buffer, size) &&
                  fillAndVerify(buffer, size, 0x3c) && copyWithinPsramAndVerify(buffer, size);
  heap_caps_free(buffer);
  snprintf(g_status, sizeof(g_status), ok ? "PASS max PSRAM block" : "FAIL see serial log");
  Serial.printf("[psram-test] done result=%s\n", ok ? "PASS" : "FAIL");
  drawPsramTest(g_status, size / 1024, size / 1024, true, ok);
  g_running = false;
}

void psramTestBegin(AppContext &context) {
  (void)context;
  snprintf(g_status, sizeof(g_status), "Press A to run max PSRAM test");
  drawPsramTest(g_status, 0, 0, false, false);
}

void psramTestTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  if (context.buttons[0].pressed) {
    runPsramTest();
  }
}

void psramTestEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

}

const LauncherApp &psramTestApp() {
  static const LauncherApp app = {"PSRAM Test", psramTestBegin, psramTestTick, psramTestEnd};
  return app;
}
