#include "mia_host_abi.h"

#include <string.h>

#include "audio_host.h"

uint8_t mia_host_audio_open(uint32_t sample_rate, uint8_t channels,
                            uint8_t bits_per_sample) {
  return hostAudioOpen(sample_rate, channels, bits_per_sample) ? 1 : 0;
}

int32_t mia_host_audio_write_pcm16(const int16_t *samples, uint32_t frame_count,
                                   uint8_t channels) {
  return hostAudioWritePcm16(samples, frame_count, channels);
}

void mia_host_audio_stop(void) { hostAudioStop(); }

void mia_host_audio_close(void) { hostAudioClose(); }

uint8_t mia_host_audio_get_status(MiaHostAudioStatus *status) {
  if (status == nullptr) {
    return 0;
  }

  HostAudioStatus audioStatus = {};
  hostAudioGetStatus(&audioStatus);
  memset(status, 0, sizeof(MiaHostAudioStatus));
  status->open = audioStatus.open ? 1 : 0;
  status->sample_rate = audioStatus.sampleRate;
  status->channels = audioStatus.channels;
  status->bits_per_sample = audioStatus.bitsPerSample;
  status->last_error = audioStatus.lastError;
  return 1;
}
