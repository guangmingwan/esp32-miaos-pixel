#include "mia_elf_runner.h"

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launcher_log.h"
#include "mia_host_abi.h"

#if __has_include("esp_elf.h") && __has_include("private/elf_symbol.h")
#include "esp_elf.h"
#include "private/elf_symbol.h"
#define MIA_HAS_ESP_ELF_LOADER 1
#else
#define MIA_HAS_ESP_ELF_LOADER 0
#endif

extern "C" float __divsf3(float, float);

#if MIA_HAS_ESP_ELF_LOADER
static constexpr char SD_VFS_ROOT[] = "/sd";

static uint32_t g_elfIoReadCount = 0;

static bool miaElfVfsPath(char *dest, size_t destSize, const char *path) {
  int written = 0;
  if (strncmp(path, "/sd/", 4) == 0) {
    written = snprintf(dest, destSize, "%s", path);
  } else if (strcmp(path, "/sd") == 0) {
    written = snprintf(dest, destSize, "%s", path);
  } else {
    const char *separator = path[0] == '/' ? "" : "/";
    written = snprintf(dest, destSize, "%s%s%s", SD_VFS_ROOT, separator, path);
  }
  return written >= 0 && static_cast<size_t>(written) < destSize;
}

extern "C" FILE *mia_elf_fopen(const char *path, const char *mode) {
  char vfsPath[160];
  launcherTracef("[elf-io] fopen path='%s' mode='%s'",
                 path == nullptr ? "<null>" : path,
                 mode == nullptr ? "<null>" : mode);
  if (path == nullptr || mode == nullptr || mode[0] != 'r' ||
      !miaElfVfsPath(vfsPath, sizeof(vfsPath), path)) {
    launcherTrace("[elf-io] fopen rejected");
    return nullptr;
  }
  FILE *file = fopen(vfsPath, "rb");
  if (file == nullptr) {
    launcherTrace("[elf-io] fopen => <null>");
    return nullptr;
  }
  launcherTracef("[elf-io] fopen => %p", file);
  return file;
}

extern "C" size_t mia_elf_fread(void *ptr, size_t size, size_t count, FILE *stream) {
  ++g_elfIoReadCount;
  if (g_elfIoReadCount <= 6 || g_elfIoReadCount % 32 == 0) {
    launcherTracef("[elf-io] fread #%u size=%u count=%u stream=%p",
                   static_cast<unsigned>(g_elfIoReadCount),
                   static_cast<unsigned>(size), static_cast<unsigned>(count), stream);
  }
  if (ptr == nullptr || stream == nullptr || size == 0 || count == 0) {
    launcherTrace("[elf-io] fread rejected");
    return 0;
  }
  const size_t itemCount = fread(ptr, size, count, stream);
  if (g_elfIoReadCount <= 6 || itemCount == 0) {
    launcherTracef("[elf-io] fread #%u => items=%u",
                   static_cast<unsigned>(g_elfIoReadCount),
                   static_cast<unsigned>(itemCount));
  }
  return itemCount;
}

extern "C" int mia_elf_fseek(FILE *stream, long offset, int origin) {
  launcherTracef("[elf-io] fseek stream=%p off=%ld whence=%d", stream, offset, origin);
  if (stream == nullptr) {
    launcherTrace("[elf-io] fseek => -1");
    return -1;
  }
  const int result = fseek(stream, offset, origin);
  launcherTracef("[elf-io] fseek => %d", result);
  return result;
}

extern "C" long mia_elf_ftell(FILE *stream) {
  launcherTracef("[elf-io] ftell stream=%p", stream);
  const long result = stream == nullptr ? -1 : ftell(stream);
  launcherTracef("[elf-io] ftell => %ld", result);
  return result;
}

extern "C" int mia_elf_fclose(FILE *stream) {
  launcherTracef("[elf-io] fclose stream=%p", stream);
  if (stream == nullptr) {
    launcherTrace("[elf-io] fclose => -1");
    return -1;
  }
  const int result = fclose(stream);
  launcherTracef("[elf-io] fclose => %d", result);
  return result;
}

static const struct esp_elfsym MIA_HOST_SYMBOLS[] = {
    ESP_ELFSYM_EXPORT(memcpy),
    ESP_ELFSYM_EXPORT(memcmp),
    ESP_ELFSYM_EXPORT(memmove),
    ESP_ELFSYM_EXPORT(memset),
    ESP_ELFSYM_EXPORT(malloc),
    ESP_ELFSYM_EXPORT(realloc),
    ESP_ELFSYM_EXPORT(free),
    {.name = "fopen", .sym = reinterpret_cast<const void *>(mia_elf_fopen)},
    {.name = "fread", .sym = reinterpret_cast<const void *>(mia_elf_fread)},
    {.name = "fseek", .sym = reinterpret_cast<const void *>(mia_elf_fseek)},
    {.name = "ftell", .sym = reinterpret_cast<const void *>(mia_elf_ftell)},
    {.name = "fclose", .sym = reinterpret_cast<const void *>(mia_elf_fclose)},
    ESP_ELFSYM_EXPORT(strcmp),
    ESP_ELFSYM_EXPORT(snprintf),
    ESP_ELFSYM_EXPORT(strncpy),
    ESP_ELFSYM_EXPORT(strrchr),
    ESP_ELFSYM_EXPORT(__divsf3),
    ESP_ELFSYM_EXPORT(mia_host_abi_version),
    ESP_ELFSYM_EXPORT(mia_host_log),
    ESP_ELFSYM_EXPORT(mia_host_screen_width),
    ESP_ELFSYM_EXPORT(mia_host_screen_height),
    ESP_ELFSYM_EXPORT(mia_host_clear),
    ESP_ELFSYM_EXPORT(mia_host_fill_rect),
    ESP_ELFSYM_EXPORT(mia_host_fill_screen_rgb565),
    ESP_ELFSYM_EXPORT(mia_host_draw_text),
    ESP_ELFSYM_EXPORT(mia_host_present),
    ESP_ELFSYM_EXPORT(mia_host_buttons_poll),
    ESP_ELFSYM_EXPORT(mia_host_button_down),
    ESP_ELFSYM_EXPORT(mia_host_button_pressed),
    ESP_ELFSYM_EXPORT(mia_host_button_released),
    ESP_ELFSYM_EXPORT(mia_host_delay_ms),
    ESP_ELFSYM_EXPORT(mia_host_millis),
    ESP_ELFSYM_EXPORT(mia_host_backlight_set),
    ESP_ELFSYM_EXPORT(mia_host_rtc_read),
    ESP_ELFSYM_EXPORT(mia_host_rtc_write),
    ESP_ELFSYM_EXPORT(mia_host_rtc_days_in_month),
    ESP_ELFSYM_EXPORT(mia_host_rtc_day_of_week),
    ESP_ELFSYM_EXPORT(mia_host_sd_list_dir),
    ESP_ELFSYM_EXPORT(mia_host_get_system_info),
    ESP_ELFSYM_EXPORT(mia_host_read_battery),
    ESP_ELFSYM_EXPORT(mia_host_wifi_scan),
    ESP_ELFSYM_EXPORT(mia_host_wifi_off),
    ESP_ELFSYM_EXPORT(mia_host_wifi_files_start),
    ESP_ELFSYM_EXPORT(mia_host_wifi_files_poll),
    ESP_ELFSYM_EXPORT(mia_host_wifi_files_stop),
    ESP_ELFSYM_EXPORT(mia_host_wifi_files_get_status),
    ESP_ELFSYM_EXPORT(mia_host_ftp_start),
    ESP_ELFSYM_EXPORT(mia_host_ftp_poll),
    ESP_ELFSYM_EXPORT(mia_host_ftp_stop),
    ESP_ELFSYM_EXPORT(mia_host_ftp_get_status),
    ESP_ELFSYM_EXPORT(mia_host_audio_open),
    ESP_ELFSYM_EXPORT(mia_host_audio_write_pcm16),
    ESP_ELFSYM_EXPORT(mia_host_audio_stop),
    ESP_ELFSYM_EXPORT(mia_host_audio_close),
    ESP_ELFSYM_EXPORT(mia_host_audio_get_status),
    ESP_ELFSYM_END,
};

// ELF apps run in a dedicated FreeRTOS task instead of the main task.
// The main task stack is only 8 KiB (CONFIG_ESP_MAIN_TASK_STACK_SIZE), which is
// far too small for deep-stack apps such as the music player: minimp3 alone
// peaks at ~10-15 KiB of stack per decode call, and the overflow triggers a
// "stack overflow in task main" panic followed by a reboot.
//
// Running each ELF app on its own task gives it a generous isolated stack so
// decoder-heavy apps survive, and a crash inside the ELF app cannot directly
// corrupt the launcher's own stack.
static constexpr uint32_t ELF_RUN_TASK_STACK = 64 * 1024;
static constexpr UBaseType_t ELF_RUN_TASK_PRIORITY = 1;

struct ElfRunContext {
  const uint8_t *elfData;
  int err;
  SemaphoreHandle_t done;
};

static void elfRunTask(void *arg) {
  ElfRunContext *ctx = static_cast<ElfRunContext *>(arg);
  esp_elf_t elf;
  g_elfIoReadCount = 0;
  launcherTracef("[elf] task entry core=%d", static_cast<int>(xPortGetCoreID()));

  int err = esp_elf_init(&elf);
  launcherTracef("[elf] esp_elf_init => %d", err);
  if (err == 0) {
    err = esp_elf_relocate(&elf, ctx->elfData);
    launcherTracef("[elf] esp_elf_relocate => %d", err);
  }
  if (err == 0) {
    launcherTracef("[elf] relocated entry=%p", reinterpret_cast<void *>(elf.entry));
  }
  mia_host_buttons_poll();
  if (err == 0) {
    char *argv[] = {const_cast<char *>("sd-app")};
    launcherTrace("[elf] calling esp_elf_request");
    err = esp_elf_request(&elf, 0, 1, argv);
    launcherTracef("[elf] esp_elf_request => %d", err);
  }
  esp_elf_deinit(&elf);
  launcherTrace("[elf] esp_elf_deinit done");
  ctx->err = err;
  xSemaphoreGive(ctx->done);
  vTaskDelete(nullptr);
}
#endif

static MiaElfRunResult readElfFile(const char *path, uint8_t **data) {
  char vfsPath[160];
  launcherTracef("[elf] open path='%s'", path == nullptr ? "<null>" : path);
  if (path == nullptr || !miaElfVfsPath(vfsPath, sizeof(vfsPath), path)) {
    launcherTrace("[elf] invalid path");
    return {MiaElfRunStatus::ReadError, -1};
  }

  FILE *file = fopen(vfsPath, "rb");
  if (file == nullptr) {
    launcherTrace("[elf] open failed");
    return {MiaElfRunStatus::ReadError, -1};
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    launcherTrace("[elf] size seek failed");
    return {MiaElfRunStatus::ReadError, -2};
  }
  const long fileSize = ftell(file);
  if (fileSize <= 0) {
    fclose(file);
    launcherTrace("[elf] reject empty file");
    return {MiaElfRunStatus::ReadError, -2};
  }
  rewind(file);

  const size_t size = static_cast<size_t>(fileSize);
  launcherTracef("[elf] file size=%u", static_cast<unsigned>(size));

  uint8_t *buffer = static_cast<uint8_t *>(
      heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (buffer == nullptr) {
    fclose(file);
    launcherTracef("[elf] internal alloc failed size=%u", static_cast<unsigned>(size));
    return {MiaElfRunStatus::ReadError, -3};
  }
  launcherTracef("[elf] internal malloc ok ptr=%p", buffer);

  launcherTracef("[elf] read start size=%u", static_cast<unsigned>(size));
  const size_t readCount = fread(buffer, 1, size, file);
  fclose(file);
  launcherTracef("[elf] read bytes=%u expected=%u", static_cast<unsigned>(readCount),
                 static_cast<unsigned>(size));
  if (readCount != size) {
    free(buffer);
    launcherTrace("[elf] read size mismatch");
    return {MiaElfRunStatus::ReadError, -4};
  }

  *data = buffer;
  launcherTrace("[elf] read complete");
  return {MiaElfRunStatus::Ok, 0};
}

MiaElfRunResult miaRunElfApp(const char *path, bool sdReady) {
  launcherTracef("[elf] run request path='%s' sdReady=%d loader=%d",
                 path == nullptr ? "<null>" : path, sdReady ? 1 : 0,
                 MIA_HAS_ESP_ELF_LOADER);
  if (!sdReady) {
    launcherTrace("[elf] abort: SD unavailable");
    return {MiaElfRunStatus::SdUnavailable, 0};
  }
  if (path == nullptr || path[0] == '\0') {
    launcherTrace("[elf] abort: empty path");
    return {MiaElfRunStatus::ReadError, -5};
  }

  uint8_t *elfData = nullptr;
  MiaElfRunResult result = readElfFile(path, &elfData);
  if (result.status != MiaElfRunStatus::Ok) {
    launcherTracef("[elf] read failed status=%s code=%d", miaElfRunStatusText(result.status),
                   result.errorCode);
    return result;
  }

#if MIA_HAS_ESP_ELF_LOADER
  launcherTrace("[elf] register host symbols");
  esp_elf_register_symbol(MIA_HOST_SYMBOLS);

  int err = 0;
  SemaphoreHandle_t done = xSemaphoreCreateBinary();
  if (done == nullptr) {
    launcherTrace("[elf] failed to create completion semaphore");
    err = -ENOMEM;
  } else {
    ElfRunContext ctx = {elfData, 0, done};
    const BaseType_t elfTaskCore = 1;
    const BaseType_t created =
        xTaskCreatePinnedToCore(elfRunTask, "elf-app", ELF_RUN_TASK_STACK,
                                &ctx, ELF_RUN_TASK_PRIORITY, nullptr, elfTaskCore);
    if (created != pdPASS) {
      launcherTracef("[elf] failed to create elf-app task stack=%u core=%d",
                     static_cast<unsigned>(ELF_RUN_TASK_STACK), static_cast<int>(elfTaskCore));
      err = -ENOMEM;
    } else {
      xSemaphoreTake(done, portMAX_DELAY);
      err = ctx.err;
    }
    vSemaphoreDelete(done);
  }

  esp_elf_unregister_symbol(MIA_HOST_SYMBOLS);
  launcherTrace("[elf] unregister host symbols");
  free(elfData);
  launcherTracef("[elf] run done err=%d", err);
  return {err == 0 ? MiaElfRunStatus::Ok : MiaElfRunStatus::RunError, err};
#else
  free(elfData);
  launcherTrace("[elf] loader unavailable at compile time");
  return {MiaElfRunStatus::RunError, -6};
#endif
}

const char *miaElfRunStatusText(MiaElfRunStatus status) {
  switch (status) {
    case MiaElfRunStatus::Ok:
      return "OK";
    case MiaElfRunStatus::SdUnavailable:
      return "SD unavailable";
    case MiaElfRunStatus::ReadError:
      return "ELF read error";
    case MiaElfRunStatus::RunError:
      return "ELF run error";
  }
  return "ELF unknown";
}
