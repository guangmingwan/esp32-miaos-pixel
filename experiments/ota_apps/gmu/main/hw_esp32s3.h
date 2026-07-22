#pragma once

#define SAMPLE_BUFFER_SIZE 1024

int hw_open_mixer(int mixer_channel);
void hw_close_mixer(void);
void hw_set_volume(int volume);
void hw_display_off(void);
void hw_display_on(void);
void hw_detect_device_model(void);
const char *hw_get_device_model_name(void);
