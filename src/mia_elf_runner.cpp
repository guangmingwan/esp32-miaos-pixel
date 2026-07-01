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
    ESP_ELFSYM_END,
};
#endif

static MiaElfRunResult readElfFile(const char *path, uint8_t **data) {
  File file = SD.open(path, FILE_READ);
  if (!file) {
    return {MiaElfRunStatus::ReadError, -1};
  }

  const size_t size = file.size();
  if (size == 0) {
    file.close();
    return {MiaElfRunStatus::ReadError, -2};
  }

  uint8_t *buffer = static_cast<uint8_t *>(malloc(size));
  if (buffer == nullptr) {
    file.close();
    return {MiaElfRunStatus::ReadError, -3};
  }

  const size_t readCount = file.read(buffer, size);
  file.close();
  if (readCount != size) {
    free(buffer);
    return {MiaElfRunStatus::ReadError, -4};
  }

  *data = buffer;
  return {MiaElfRunStatus::Ok, 0};
}

MiaElfRunResult miaRunElfApp(const char *path, bool sdReady) {
  if (!sdReady) {
    return {MiaElfRunStatus::SdUnavailable, 0};
  }
  if (path == nullptr || path[0] == '\0') {
    return {MiaElfRunStatus::ReadError, -5};
  }

  uint8_t *elfData = nullptr;
  MiaElfRunResult result = readElfFile(path, &elfData);
  if (result.status != MiaElfRunStatus::Ok) {
    return result;
  }

#if MIA_HAS_ESP_ELF_LOADER
  esp_elf_register_symbol(MIA_HOST_SYMBOLS);

  esp_elf_t elf;
  int err = esp_elf_init(&elf);
  if (err == 0) {
    err = esp_elf_relocate(&elf, elfData);
    if (err == 0) {
      char *argv[] = {const_cast<char *>("sd-app")};
      err = esp_elf_request(&elf, 0, 1, argv);
    }
    esp_elf_deinit(&elf);
  }

  esp_elf_unregister_symbol(MIA_HOST_SYMBOLS);
  free(elfData);
  return {err == 0 ? MiaElfRunStatus::Ok : MiaElfRunStatus::RunError, err};
#else
  free(elfData);
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
