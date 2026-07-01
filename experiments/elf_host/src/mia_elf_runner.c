#include "mia_elf_runner.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_elf.h"
#include "esp_log.h"
#include "mia_host_abi.h"
#include "private/elf_symbol.h"

static const char *TAG = "mia_elf_runner";
static const char *APP_ROOT = "/sdcard/MiaOS/Application";
static const char *APP_ELF_NAME = "app.elf";

static const struct esp_elfsym MIA_HOST_SYMBOLS[] = {
    ESP_ELFSYM_EXPORT(mia_host_abi_version),
    ESP_ELFSYM_EXPORT(mia_host_log),
    ESP_ELFSYM_END,
};

static bool ends_with(const char *value, const char *suffix) {
  const size_t value_len = strlen(value);
  const size_t suffix_len = strlen(suffix);
  if (value_len < suffix_len) {
    return false;
  }
  return strcmp(value + value_len - suffix_len, suffix) == 0;
}

static esp_err_t find_first_app(char *path, size_t path_size) {
  DIR *root = opendir(APP_ROOT);
  if (root == NULL) {
    ESP_LOGE(TAG, "Failed to open %s", APP_ROOT);
    return ESP_FAIL;
  }

  struct dirent *entry;
  while ((entry = readdir(root)) != NULL) {
    if (!ends_with(entry->d_name, ".app")) {
      continue;
    }
    const int written = snprintf(path, path_size, "%s/%s/%s", APP_ROOT, entry->d_name,
                                 APP_ELF_NAME);
    if (written < 0 || (size_t)written >= path_size) {
      closedir(root);
      return ESP_ERR_NO_MEM;
    }
    FILE *file = fopen(path, "rb");
    if (file != NULL) {
      fclose(file);
      closedir(root);
      ESP_LOGI(TAG, "Found ELF app %s", path);
      return ESP_OK;
    }
  }

  closedir(root);
  ESP_LOGW(TAG, "No .app directory with app.elf found under %s", APP_ROOT);
  return ESP_ERR_NOT_FOUND;
}

static esp_err_t read_file(const char *path, uint8_t **data) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open %s", path);
    return ESP_FAIL;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return ESP_FAIL;
  }
  const long size = ftell(file);
  if (size <= 0) {
    fclose(file);
    return ESP_ERR_INVALID_SIZE;
  }
  rewind(file);

  uint8_t *buffer = malloc((size_t)size);
  if (buffer == NULL) {
    fclose(file);
    return ESP_ERR_NO_MEM;
  }
  const size_t read_count = fread(buffer, 1, (size_t)size, file);
  fclose(file);
  if (read_count != (size_t)size) {
    free(buffer);
    return ESP_FAIL;
  }
  *data = buffer;
  ESP_LOGI(TAG, "Loaded %ld bytes from %s", size, path);
  return ESP_OK;
}

esp_err_t mia_run_first_sd_elf_app(void) {
  char path[160];
  esp_err_t err = find_first_app(path, sizeof(path));
  if (err != ESP_OK) {
    return err;
  }

  uint8_t *elf_data = NULL;
  err = read_file(path, &elf_data);
  if (err != ESP_OK) {
    return err;
  }

  esp_elf_register_symbol(MIA_HOST_SYMBOLS);

  esp_elf_t elf;
  err = esp_elf_init(&elf);
  if (err != ESP_OK) {
    free(elf_data);
    return err;
  }

  err = esp_elf_relocate(&elf, elf_data);
  if (err == ESP_OK) {
    char *argv[] = {"sd-app"};
    err = esp_elf_request(&elf, 0, 1, argv);
  }

  esp_elf_deinit(&elf);
  esp_elf_unregister_symbol(MIA_HOST_SYMBOLS);
  free(elf_data);
  return err;
}
