#include "audio_host.h"

#include <string.h>

#include <Arduino.h>
#include <driver/i2s.h>

#include "pins.h"
#include "system_settings.h"

namespace {

constexpr i2s_port_t kAudioPort = I2S_NUM_0;
constexpr uint8_t kOutputChannels = 2;
constexpr size_t kScratchFrames = 2304;

HostAudioStatus g_audioStatus = {
    .open = false,
    .sampleRate = 0,
    .channels = 0,
    .bitsPerSample = 0,
    .lastError = 0,
};

int16_t g_stereoScratch[kScratchFrames * kOutputChannels] = {};

uint32_t g_writeCallCount = 0;

bool installI2s(uint32_t sampleRate) {
  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = sampleRate,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 6,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0,
      .mclk_multiple = static_cast<i2s_mclk_multiple_t>(0),
      .bits_per_chan = static_cast<i2s_bits_per_chan_t>(0),
      .chan_mask = static_cast<i2s_channel_t>(0),
      .total_chan = 0,
      .left_align = false,
      .big_edin = false,
      .bit_order_msb = false,
      .skip_msk = false,
  };

  const i2s_pin_config_t pins = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = static_cast<gpio_num_t>(I2S_BCK_PIN),
      .ws_io_num = static_cast<gpio_num_t>(I2S_WS_PIN),
      .data_out_num = static_cast<gpio_num_t>(I2S_DATA_PIN),
      .data_in_num = I2S_PIN_NO_CHANGE,
  };

  esp_err_t err = i2s_driver_install(kAudioPort, &config, 0, nullptr);
  if (err != ESP_OK) {
    g_audioStatus.lastError = err;
    return false;
  }
  err = i2s_set_pin(kAudioPort, &pins);
  if (err != ESP_OK) {
    g_audioStatus.lastError = err;
    i2s_driver_uninstall(kAudioPort);
    return false;
  }
  err = i2s_zero_dma_buffer(kAudioPort);
  if (err != ESP_OK) {
    g_audioStatus.lastError = err;
    i2s_driver_uninstall(kAudioPort);
    return false;
  }

  digitalWrite(AMP_CTRL_PIN, HIGH);
  ESP_LOGI("AUDIO_HOST", "I2S installed: amp GPIO %d set HIGH, level=%d",
           AMP_CTRL_PIN, digitalRead(AMP_CTRL_PIN));
  return true;
}

const int16_t *expandFrames(const int16_t *samples, uint32_t frameCount,
                            uint8_t channels, uint32_t *expandedFrames) {
  const uint8_t volume = miaSystemVolume();
  if (channels == kOutputChannels && volume == 100) {
    *expandedFrames = frameCount;
    return samples;
  }
  if ((channels != 1 && channels != 2) || frameCount > kScratchFrames) {
    *expandedFrames = 0;
    return nullptr;
  }

  for (uint32_t index = 0; index < frameCount; ++index) {
    const int32_t left = channels == 1 ? samples[index] : samples[index * 2];
    const int32_t right = channels == 1 ? samples[index] : samples[index * 2 + 1];
    g_stereoScratch[index * 2] = static_cast<int16_t>(left * volume / 100);
    g_stereoScratch[index * 2 + 1] = static_cast<int16_t>(right * volume / 100);
  }
  *expandedFrames = frameCount;
  return g_stereoScratch;
}

}

bool hostAudioOpen(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample) {
  if (sampleRate == 0 || (channels != 1 && channels != 2) || bitsPerSample != 16) {
    g_audioStatus.lastError = ESP_ERR_INVALID_ARG;
    return false;
  }

  if (g_audioStatus.open) {
    hostAudioClose();
  }

  if (!installI2s(sampleRate)) {
    return false;
  }

  g_writeCallCount = 0;
  g_audioStatus.open = true;
  g_audioStatus.sampleRate = sampleRate;
  g_audioStatus.channels = channels;
  g_audioStatus.bitsPerSample = bitsPerSample;
  g_audioStatus.lastError = ESP_OK;
  return true;
}

int32_t hostAudioWritePcm16(const int16_t *samples, uint32_t frameCount,
                            uint8_t channels) {
  ++g_writeCallCount;
  if (!g_audioStatus.open || samples == nullptr || frameCount == 0) {
    return -1;
  }

  if (g_writeCallCount <= 10) {
    int32_t sum = 0;
    uint32_t checkCount = frameCount * channels;
    if (checkCount > 256) checkCount = 256;
    for (uint32_t i = 0; i < checkCount; i++) sum += abs(samples[i]);
    ESP_LOGI("AUDIO_HOST", "write #%u: frames=%u ch=%u sumAbs=%d",
             g_writeCallCount, frameCount, channels, sum);
  }

  uint32_t expandedFrames = 0;
  const int16_t *output = expandFrames(samples, frameCount, channels, &expandedFrames);
  if (output == nullptr || expandedFrames == 0) {
    g_audioStatus.lastError = ESP_ERR_INVALID_ARG;
    return -1;
  }

  const size_t bytesToWrite = static_cast<size_t>(expandedFrames) *
                              kOutputChannels * sizeof(int16_t);
  size_t bytesWritten = 0;
  const esp_err_t err = i2s_write(kAudioPort, output, bytesToWrite, &bytesWritten,
                                  pdMS_TO_TICKS(250));
  if (err != ESP_OK) {
    g_audioStatus.lastError = err;
    return -1;
  }

  g_audioStatus.lastError = ESP_OK;
  return static_cast<int32_t>(bytesWritten / (kOutputChannels * sizeof(int16_t)));
}

void hostAudioStop(void) {
  if (!g_audioStatus.open) {
    return;
  }
  i2s_zero_dma_buffer(kAudioPort);
}

void hostAudioClose(void) {
  if (!g_audioStatus.open) {
    return;
  }

  i2s_zero_dma_buffer(kAudioPort);
  i2s_driver_uninstall(kAudioPort);
  digitalWrite(AMP_CTRL_PIN, LOW);
  memset(&g_audioStatus, 0, sizeof(g_audioStatus));
}

void hostAudioGetStatus(HostAudioStatus *status) {
  if (status == nullptr) {
    return;
  }
  *status = g_audioStatus;
}
