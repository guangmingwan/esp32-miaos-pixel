#include "hw_esp32s3.h"

void hw_detect_device_model(void) {}
const char *hw_get_device_model_name(void) { return "ESP32-S3 Retro-Pixel"; }
int hw_open_mixer(int mixer_channel) { (void)mixer_channel; return 0; }
void hw_close_mixer(void) {}
void hw_set_volume(int volume) { (void)volume; }
void hw_display_off(void) {}
void hw_display_on(void) {}
