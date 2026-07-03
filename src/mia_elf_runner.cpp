#include "mia_elf_runner.h"

#include <SD.h>
#include <stdlib.h>

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
    ESP_ELFSYM_EXPORT(mia_host_draw_text),
    ESP_ELFSYM_EXPORT(mia_host_present),
    ESP_ELFSYM_EXPORT(mia_host_button_down),
    ESP_ELFSYM_EXPORT(mia_host_delay_ms),
    ESP_ELFSYM_EXPORT(mia_host_millis),
    ESP_ELFSYM_EXPORT(mia_host_rtc_read),
    ESP_ELFSYM_EXPORT(mia_host_rtc_write),
    ESP_ELFSYM_EXPORT(mia_host_rtc_days_in_month),
    ESP_ELFSYM_EXPORT(mia_host_rtc_day_of_week),
    ESP_ELFSYM_EXPORT(mia_host_sd_list_dir),
    ESP_ELFSYM_END,
};
#endif

static MiaElfRunResult readElfFile(const char *path, uint8_t **data) {
  Serial.printf("[elf] open path='%s'\n", path == nullptr ? "<null>" : path);
  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.println("[elf] open failed");
    return {MiaElfRunStatus::ReadError, -1};
  }

  const size_t size = file.size();
  Serial.printf("[elf] file size=%u\n", static_cast<unsigned>(size));
  if (size == 0) {
    file.close();
    Serial.println("[elf] reject empty file");
    return {MiaElfRunStatus::ReadError, -2};
  }

  uint8_t *buffer = static_cast<uint8_t *>(malloc(size));
  if (buffer == nullptr) {
    file.close();
    Serial.printf("[elf] malloc failed size=%u\n", static_cast<unsigned>(size));
    return {MiaElfRunStatus::ReadError, -3};
  }
  Serial.printf("[elf] malloc ok ptr=%p\n", buffer);

  const size_t readCount = file.read(buffer, size);
  file.close();
  Serial.printf("[elf] read bytes=%u expected=%u\n", static_cast<unsigned>(readCount),
                static_cast<unsigned>(size));
  if (readCount != size) {
    free(buffer);
    Serial.println("[elf] read size mismatch");
    return {MiaElfRunStatus::ReadError, -4};
  }

  *data = buffer;
  Serial.println("[elf] read complete");
  return {MiaElfRunStatus::Ok, 0};
}

MiaElfRunResult miaRunElfApp(const char *path, bool sdReady) {
  Serial.printf("[elf] run request path='%s' sdReady=%d loader=%d\n",
                path == nullptr ? "<null>" : path, sdReady ? 1 : 0,
                MIA_HAS_ESP_ELF_LOADER);
  if (!sdReady) {
    Serial.println("[elf] abort: SD unavailable");
    return {MiaElfRunStatus::SdUnavailable, 0};
  }
  if (path == nullptr || path[0] == '\0') {
    Serial.println("[elf] abort: empty path");
    return {MiaElfRunStatus::ReadError, -5};
  }

  uint8_t *elfData = nullptr;
  MiaElfRunResult result = readElfFile(path, &elfData);
  if (result.status != MiaElfRunStatus::Ok) {
    Serial.printf("[elf] read failed status=%s code=%d\n", miaElfRunStatusText(result.status),
                  result.errorCode);
    return result;
  }

#if MIA_HAS_ESP_ELF_LOADER
  Serial.println("[elf] register host symbols");
  esp_elf_register_symbol(MIA_HOST_SYMBOLS);

  esp_elf_t elf;
  int err = esp_elf_init(&elf);
  Serial.printf("[elf] esp_elf_init => %d\n", err);
  if (err == 0) {
    err = esp_elf_relocate(&elf, elfData);
    Serial.printf("[elf] esp_elf_relocate => %d\n", err);
    if (err == 0) {
      char *argv[] = {const_cast<char *>("sd-app")};
      err = esp_elf_request(&elf, 0, 1, argv);
      Serial.printf("[elf] esp_elf_request => %d\n", err);
    }
    esp_elf_deinit(&elf);
    Serial.println("[elf] esp_elf_deinit done");
  }

  esp_elf_unregister_symbol(MIA_HOST_SYMBOLS);
  Serial.println("[elf] unregister host symbols");
  free(elfData);
  Serial.printf("[elf] run done err=%d\n", err);
  return {err == 0 ? MiaElfRunStatus::Ok : MiaElfRunStatus::RunError, err};
#else
  free(elfData);
  Serial.println("[elf] loader unavailable at compile time");
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
