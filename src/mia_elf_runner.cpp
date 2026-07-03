#include "mia_elf_runner.h"

#include <SD.h>
#include <stdlib.h>

#include "launcher_log.h"
#include "mia_host_abi.h"

#if __has_include("esp_elf.h") && __has_include("private/elf_symbol.h")
#include "esp_elf.h"
#include "private/elf_symbol.h"
#define MIA_HAS_ESP_ELF_LOADER 1
#else
#define MIA_HAS_ESP_ELF_LOADER 0
#endif

#if MIA_HAS_ESP_ELF_LOADER
static const struct esp_elfsym MIA_HOST_SYMBOLS[] = {
    ESP_ELFSYM_EXPORT(mia_host_abi_version),
    ESP_ELFSYM_EXPORT(mia_host_log),
    ESP_ELFSYM_EXPORT(mia_host_screen_width),
    ESP_ELFSYM_EXPORT(mia_host_screen_height),
    ESP_ELFSYM_EXPORT(mia_host_clear),
    ESP_ELFSYM_EXPORT(mia_host_fill_rect),
    ESP_ELFSYM_EXPORT(mia_host_fill_screen_rgb565),
    ESP_ELFSYM_EXPORT(mia_host_draw_text),
    ESP_ELFSYM_EXPORT(mia_host_present),
    ESP_ELFSYM_EXPORT(mia_host_button_down),
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
    ESP_ELFSYM_END,
};
#endif

static MiaElfRunResult readElfFile(const char *path, uint8_t **data) {
  launcherTracef("[elf] open path='%s'", path == nullptr ? "<null>" : path);
  File file = SD.open(path, FILE_READ);
  if (!file) {
    launcherTrace("[elf] open failed");
    return {MiaElfRunStatus::ReadError, -1};
  }

  const size_t size = file.size();
  launcherTracef("[elf] file size=%u", static_cast<unsigned>(size));
  if (size == 0) {
    file.close();
    launcherTrace("[elf] reject empty file");
    return {MiaElfRunStatus::ReadError, -2};
  }

  uint8_t *buffer = static_cast<uint8_t *>(malloc(size));
  if (buffer == nullptr) {
    file.close();
    launcherTracef("[elf] malloc failed size=%u", static_cast<unsigned>(size));
    return {MiaElfRunStatus::ReadError, -3};
  }
  launcherTracef("[elf] malloc ok ptr=%p", buffer);

  const size_t readCount = file.read(buffer, size);
  file.close();
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

  esp_elf_t elf;
  int err = esp_elf_init(&elf);
  launcherTracef("[elf] esp_elf_init => %d", err);
  if (err == 0) {
    err = esp_elf_relocate(&elf, elfData);
    launcherTracef("[elf] esp_elf_relocate => %d", err);
    if (err == 0) {
      char *argv[] = {const_cast<char *>("sd-app")};
      err = esp_elf_request(&elf, 0, 1, argv);
      launcherTracef("[elf] esp_elf_request => %d", err);
    }
    esp_elf_deinit(&elf);
    launcherTrace("[elf] esp_elf_deinit done");
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
