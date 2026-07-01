#include "mia_host_abi.h"

#include <Arduino.h>

uint32_t mia_host_abi_version(void) { return 1; }

void mia_host_log(const char *message) {
  Serial.printf("[mia_host_abi] %s\n", message == nullptr ? "<null>" : message);
}
