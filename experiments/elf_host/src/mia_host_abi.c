#include "mia_host_abi.h"

#include "esp_log.h"

static const char *TAG = "mia_host_abi";

uint32_t mia_host_abi_version(void) { return 1; }

void mia_host_log(const char *message) {
  ESP_LOGI(TAG, "%s", message == NULL ? "<null>" : message);
}
