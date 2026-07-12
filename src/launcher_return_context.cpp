#include "launcher_return_context.h"

#include <cstring>

#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "launcher_log.h"

namespace {

constexpr const char *NVS_NAMESPACE = "mia-return";
constexpr const char *VALID_KEY = "valid";
constexpr const char *CATEGORY_KEY = "category";
constexpr const char *NAME_KEY = "name";

esp_err_t openStore(nvs_open_mode_t mode, nvs_handle_t *handle) {
  esp_err_t err = nvs_open(NVS_NAMESPACE, mode, handle);
  if (err != ESP_ERR_NVS_NOT_INITIALIZED) return err;

  err = nvs_flash_init();
  if (err != ESP_OK) return err;
  return nvs_open(NVS_NAMESPACE, mode, handle);
}

bool validText(const char *text, size_t capacity) {
  return text != nullptr && text[0] != '\0' && strnlen(text, capacity) < capacity;
}

}  // namespace

bool miaLauncherReturnContextSave(const char *category, const char *name) {
  if (!validText(category, MIA_MANIFEST_CATEGORY_SIZE) ||
      !validText(name, MIA_MANIFEST_NAME_SIZE)) {
    launcherTrace("[return-context] invalid category/name");
    return false;
  }

  nvs_handle_t handle;
  esp_err_t err = openStore(NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    launcherTracef("[return-context] open for save failed: %s", esp_err_to_name(err));
    return false;
  }

  // Commit an invalid marker first so interrupted updates cannot expose stale text.
  err = nvs_set_u8(handle, VALID_KEY, 0);
  if (err == ESP_OK) err = nvs_commit(handle);
  if (err == ESP_OK) err = nvs_set_str(handle, CATEGORY_KEY, category);
  if (err == ESP_OK) err = nvs_set_str(handle, NAME_KEY, name);
  if (err == ESP_OK) err = nvs_set_u8(handle, VALID_KEY, 1);
  if (err == ESP_OK) err = nvs_commit(handle);
  nvs_close(handle);

  launcherTracef("[return-context] save category='%s' name='%s': %s", category, name,
                 esp_err_to_name(err));
  return err == ESP_OK;
}

bool miaLauncherReturnContextLoad(LauncherReturnContext *context) {
  if (context == nullptr) return false;
  *context = {};

  nvs_handle_t handle;
  esp_err_t err = openStore(NVS_READONLY, &handle);
  if (err != ESP_OK) return false;

  uint8_t valid = 0;
  size_t categorySize = sizeof(context->category);
  size_t nameSize = sizeof(context->name);
  err = nvs_get_u8(handle, VALID_KEY, &valid);
  if (err == ESP_OK && valid == 1) {
    err = nvs_get_str(handle, CATEGORY_KEY, context->category, &categorySize);
  }
  if (err == ESP_OK && valid == 1) {
    err = nvs_get_str(handle, NAME_KEY, context->name, &nameSize);
  }
  nvs_close(handle);

  const bool loaded = err == ESP_OK && valid == 1 &&
                      validText(context->category, sizeof(context->category)) &&
                      validText(context->name, sizeof(context->name));
  if (loaded) {
    launcherTracef("[return-context] load category='%s' name='%s'", context->category,
                   context->name);
  }
  return loaded;
}

bool miaLauncherReturnContextClear() {
  nvs_handle_t handle;
  esp_err_t err = openStore(NVS_READWRITE, &handle);
  if (err != ESP_OK) return err == ESP_ERR_NVS_NOT_FOUND;

  err = nvs_erase_all(handle);
  if (err == ESP_OK) err = nvs_commit(handle);
  nvs_close(handle);
  launcherTracef("[return-context] clear: %s", esp_err_to_name(err));
  return err == ESP_OK;
}
