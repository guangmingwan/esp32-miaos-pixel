#pragma once

#include <stdint.h>

void miaSystemSettingsInit(bool skipPersisted);

uint8_t miaSystemBrightness(void);
bool miaSystemSetBrightness(uint8_t brightness);
void miaSystemSetBacklightEnabled(bool enabled);

uint8_t miaSystemVolume(void);
bool miaSystemSetVolume(uint8_t volume);

bool miaSystemKeyBeep(void);
bool miaSystemSetKeyBeep(bool enabled);
