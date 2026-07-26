#pragma once

#include <stdint.h>

void miaSystemSettingsInit(bool skipPersisted);

uint8_t miaSystemBrightness(void);
bool miaSystemSetBrightness(uint8_t brightness);
void miaSystemSetBacklightEnabled(bool enabled);
uint8_t miaSystemIdleTimeoutMinutes(void);
bool miaSystemSetIdleTimeoutMinutes(uint8_t minutes);
void miaSystemIdleTick(uint32_t nowMs, bool userActivity);

uint8_t miaSystemVolume(void);
bool miaSystemSetVolume(uint8_t volume);

bool miaSystemKeyBeep(void);
bool miaSystemSetKeyBeep(bool enabled);
