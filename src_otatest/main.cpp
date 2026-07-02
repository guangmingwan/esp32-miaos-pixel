#include <Arduino.h>
#include <esp_ota_ops.h>

#define TFT_BL_PIN 13

void setup() {
  // Kill Task WDT immediately
  disableCore0WDT();
  disableCore1WDT();

  Serial.begin(115200);
  delay(2000);
  Serial.println("=== OTA TEST FIRMWARE ===");

  // Mark this OTA slot valid
  esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
  Serial.printf("mark_valid => %s\n", esp_err_to_name(err));

  // Backlight on for visual feedback
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);

  // Also show which partition we booted from
  const esp_partition_t *part = esp_ota_get_running_partition();
  if (part) {
    Serial.printf("Running from: %s at 0x%08x\n", part->label, (unsigned)part->address);
  }

  Serial.println("OTA test firmware ready - will print every 2s");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 2000) {
    last = millis();
    Serial.printf("alive @ %u ms, heap=%u\n", (unsigned)millis(), (unsigned)ESP.getFreeHeap());
  }
  delay(100);
}
