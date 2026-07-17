#include "mia_text_runtime.h"

#include <stddef.h>

#include <esp_dlfcn.h>
#include <esp_log.h>
#include <soc/soc.h>

#include "mia_text_api.h"

namespace {

constexpr const char *TAG = "mia_text";
void *g_handle = nullptr;
const MiaTextApi *g_api = nullptr;
bool g_attempted = false;

void *executable_symbol(void *symbol) {
#if defined(CONFIG_IDF_TARGET_ESP32S3) && defined(CONFIG_ELF_LOADER_CACHE_OFFSET)
  const uintptr_t address = reinterpret_cast<uintptr_t>(symbol);
  if (address >= SOC_DROM_LOW && address < SOC_DROM_HIGH) {
    return reinterpret_cast<void *>(address + (SOC_IROM_LOW - SOC_DROM_LOW));
  }
#endif
  return symbol;
}

}  // namespace

extern "C" int mia_text_runtime_init(void) {
  if (g_api != nullptr) return 1;
  if (g_attempted) return 0;
  g_attempted = true;

  g_handle = dlopen(MIA_TEXT_LIBRARY_NAME, RTLD_NOW);
  if (g_handle == nullptr) {
    const char *error = dlerror();
    ESP_LOGW(TAG, "Failed to load %s: %s", MIA_TEXT_LIBRARY_PATH,
             error != nullptr ? error : "unknown error");
    return 0;
  }

  auto get_api = reinterpret_cast<MiaTextGetApiFn>(
      executable_symbol(dlsym(g_handle, "mia_text_get_api")));
  if (get_api == nullptr) {
    const char *error = dlerror();
    ESP_LOGW(TAG, "mia_text_get_api missing: %s",
             error != nullptr ? error : "unknown error");
    dlclose(g_handle);
    g_handle = nullptr;
    return 0;
  }

  const MiaTextApi *api = get_api(MIA_TEXT_ABI_VERSION);
  if (api == nullptr || api->abi_version != MIA_TEXT_ABI_VERSION ||
      api->struct_size < offsetof(MiaTextApi, draw_text) + sizeof(api->draw_text) ||
      api->draw_text == nullptr) {
    ESP_LOGW(TAG, "Unsupported text library ABI");
    dlclose(g_handle);
    g_handle = nullptr;
    return 0;
  }

  g_api = api;
  ESP_LOGI(TAG, "Loaded %s", MIA_TEXT_LIBRARY_PATH);
  return 1;
}

extern "C" int32_t mia_text_runtime_draw_text(
    uint8_t *pixels, int32_t surface_width, int32_t surface_height, int32_t x,
    int32_t y, const char *text, uint8_t fg, uint8_t bg) {
  if (g_api == nullptr || g_api->draw_text == nullptr) return -1;
  return g_api->draw_text(pixels, surface_width, surface_height, x, y, text, fg, bg);
}
