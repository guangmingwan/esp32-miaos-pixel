#pragma once

#include <stdint.h>

struct HostAudioStatus {
  bool open;
  uint32_t sampleRate;
  uint8_t channels;
  uint8_t bitsPerSample;
  int lastError;
};

bool hostAudioOpen(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample);
int32_t hostAudioWritePcm16(const int16_t *samples, uint32_t frameCount,
                            uint8_t channels);
void hostAudioStop(void);
void hostAudioClose(void);
void hostAudioGetStatus(HostAudioStatus *status);
