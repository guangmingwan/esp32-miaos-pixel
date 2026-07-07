#include "display_host.h"
#include "mia_host_abi.h"

#include <dirent.h>
#include <driver/gpio.h>
#include <driver/i2s.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_vfs_fat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdmmc_cmd.h>
#include <string.h>
#include <sys/stat.h>

namespace {

constexpr gpio_num_t SD_MISO_PIN = GPIO_NUM_15;
constexpr gpio_num_t SD_MOSI_PIN = GPIO_NUM_6;
constexpr gpio_num_t SD_SCK_PIN = GPIO_NUM_7;
constexpr gpio_num_t SD_CS_PIN = GPIO_NUM_5;

constexpr gpio_num_t I2S_WS_PIN = GPIO_NUM_42;
constexpr gpio_num_t I2S_BCK_PIN = GPIO_NUM_41;
constexpr gpio_num_t I2S_DATA_PIN = GPIO_NUM_40;
constexpr gpio_num_t AMP_CTRL_PIN = GPIO_NUM_46;

constexpr gpio_num_t KEY_BOOT_PIN = GPIO_NUM_0;
constexpr gpio_num_t KEY_M_PIN = GPIO_NUM_8;
constexpr gpio_num_t KEY_L_PIN = GPIO_NUM_17;
constexpr gpio_num_t KEY_R_PIN = GPIO_NUM_18;
constexpr gpio_num_t KEY_SELECT_PIN = GPIO_NUM_21;
constexpr gpio_num_t KEY_START_PIN = GPIO_NUM_0;

constexpr gpio_num_t HC165_PL_PIN = GPIO_NUM_2;
constexpr gpio_num_t HC165_CLK_PIN = GPIO_NUM_39;
constexpr gpio_num_t HC165_DAT_PIN = GPIO_NUM_38;

constexpr int HC165_LEFT = 0;
constexpr int HC165_DOWN = 1;
constexpr int HC165_UP = 2;
constexpr int HC165_RIGHT = 3;
constexpr int HC165_Y = 4;
constexpr int HC165_X = 5;
constexpr int HC165_A = 6;
constexpr int HC165_B = 7;

struct ButtonState {
  bool down;
  bool pressed;
  bool released;
};

struct ButtonProbe {
  gpio_num_t gpio;
  uint8_t shift_bit;
  bool direct;
};

struct AudioStatus {
  bool open;
  uint32_t sample_rate;
  uint8_t channels;
  uint8_t bits_per_sample;
  int32_t last_error;
};

static const char *const TAG = "music_host";
static const char *const ROOT_DIR = "/sd";

static const ButtonProbe BUTTONS[14] = {
    {KEY_BOOT_PIN, 0, true},   {KEY_START_PIN, 0, true}, {KEY_M_PIN, 0, true},
    {KEY_L_PIN, 0, true},      {KEY_R_PIN, 0, true},     {KEY_SELECT_PIN, 0, true},
    {GPIO_NUM_NC, HC165_A, false},   {GPIO_NUM_NC, HC165_B, false},
    {GPIO_NUM_NC, HC165_X, false},   {GPIO_NUM_NC, HC165_Y, false},
    {GPIO_NUM_NC, HC165_UP, false},  {GPIO_NUM_NC, HC165_DOWN, false},
    {GPIO_NUM_NC, HC165_LEFT, false},{GPIO_NUM_NC, HC165_RIGHT, false},
};

static ButtonState g_buttons[14];
static bool g_last_buttons[14];
static uint8_t g_hc165_state = 0xFF;
static bool g_sd_ready = false;
static sdmmc_card_t *g_card = nullptr;
static AudioStatus g_audio = {};
static int16_t g_stereo_scratch[1024 * 2] = {};

static void delay_us(uint32_t us) { ets_delay_us(us); }

static void scan_hc165() {
  gpio_set_level(HC165_PL_PIN, 0);
  delay_us(5);
  gpio_set_level(HC165_PL_PIN, 1);
  uint8_t val = 0;
  for (int i = 0; i < 8; ++i) {
    val <<= 1;
    if (gpio_get_level(HC165_DAT_PIN) == 1) {
      val |= 1;
    }
    gpio_set_level(HC165_CLK_PIN, 1);
    delay_us(2);
    gpio_set_level(HC165_CLK_PIN, 0);
    delay_us(2);
  }
  g_hc165_state = val;
}

static void update_buttons() {
  scan_hc165();
  for (size_t i = 0; i < 14; ++i) {
    bool down;
    if (BUTTONS[i].direct) {
      down = gpio_get_level(BUTTONS[i].gpio) == 0;
    } else {
      down = (g_hc165_state & (1u << BUTTONS[i].shift_bit)) == 0;
    }
    g_buttons[i].down = down;
    g_buttons[i].pressed = down && !g_last_buttons[i];
    g_buttons[i].released = !down && g_last_buttons[i];
    g_last_buttons[i] = down;
  }
}

static esp_err_t mount_sd_card() {
  spi_bus_config_t bus_cfg = {};
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
  esp_vfs_fat_mount_config_t mount_cfg = {};

  if (g_sd_ready) {
    return ESP_OK;
  }

  bus_cfg.mosi_io_num = SD_MOSI_PIN;
  bus_cfg.miso_io_num = SD_MISO_PIN;
  bus_cfg.sclk_io_num = SD_SCK_PIN;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

  host.slot = SPI3_HOST;
  host.max_freq_khz = 20000;
  slot.host_id = SPI3_HOST;
  slot.gpio_cs = SD_CS_PIN;
  mount_cfg.format_if_mount_failed = false;
  mount_cfg.max_files = 4;
  mount_cfg.allocation_unit_size = 0;

  ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(ROOT_DIR, &host, &slot, &mount_cfg, &g_card));
  g_sd_ready = true;
  return ESP_OK;
}

static bool audio_install(uint32_t sample_rate, uint8_t channels, uint8_t bits_per_sample) {
  i2s_config_t cfg = {};
  i2s_pin_config_t pins = {};
  esp_err_t err;

  if (sample_rate == 0 || (channels != 1 && channels != 2) || bits_per_sample != 16) {
    g_audio.last_error = ESP_ERR_INVALID_ARG;
    return false;
  }
  if (g_audio.open) {
    i2s_zero_dma_buffer(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_0);
    gpio_set_level(AMP_CTRL_PIN, 0);
    memset(&g_audio, 0, sizeof(g_audio));
  }

  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = sample_rate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 6;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = 0;

  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_BCK_PIN;
  pins.ws_io_num = I2S_WS_PIN;
  pins.data_out_num = I2S_DATA_PIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  err = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  if (err != ESP_OK) {
    g_audio.last_error = err;
    return false;
  }
  err = i2s_set_pin(I2S_NUM_0, &pins);
  if (err != ESP_OK) {
    g_audio.last_error = err;
    i2s_driver_uninstall(I2S_NUM_0);
    return false;
  }
  err = i2s_zero_dma_buffer(I2S_NUM_0);
  if (err != ESP_OK) {
    g_audio.last_error = err;
    i2s_driver_uninstall(I2S_NUM_0);
    return false;
  }

  gpio_set_level(AMP_CTRL_PIN, 1);
  g_audio.open = true;
  g_audio.sample_rate = sample_rate;
  g_audio.channels = channels;
  g_audio.bits_per_sample = bits_per_sample;
  g_audio.last_error = ESP_OK;
  return true;
}

static const int16_t *expand_frames(const int16_t *samples, uint32_t frame_count,
                                    uint8_t channels, uint32_t *expanded_frames) {
  if (channels == 2) {
    *expanded_frames = frame_count;
    return samples;
  }
  if (channels != 1 || frame_count > 1024) {
    *expanded_frames = 0;
    return nullptr;
  }
  for (uint32_t i = 0; i < frame_count; ++i) {
    g_stereo_scratch[i * 2] = samples[i];
    g_stereo_scratch[i * 2 + 1] = samples[i];
  }
  *expanded_frames = frame_count;
  return g_stereo_scratch;
}

}

extern "C" esp_err_t host_platform_init(void) {
  gpio_reset_pin(AMP_CTRL_PIN);
  gpio_set_direction(AMP_CTRL_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(AMP_CTRL_PIN, 0);

  gpio_reset_pin(HC165_PL_PIN);
  gpio_set_direction(HC165_PL_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(HC165_PL_PIN, 1);
  gpio_reset_pin(HC165_CLK_PIN);
  gpio_set_direction(HC165_CLK_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(HC165_CLK_PIN, 0);
  gpio_reset_pin(HC165_DAT_PIN);
  gpio_set_direction(HC165_DAT_PIN, GPIO_MODE_INPUT);

  gpio_reset_pin(KEY_BOOT_PIN);
  gpio_set_direction(KEY_BOOT_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(KEY_BOOT_PIN, GPIO_PULLUP_ONLY);
  gpio_reset_pin(KEY_M_PIN);
  gpio_set_direction(KEY_M_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(KEY_M_PIN, GPIO_PULLUP_ONLY);
  gpio_reset_pin(KEY_L_PIN);
  gpio_set_direction(KEY_L_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(KEY_L_PIN, GPIO_PULLUP_ONLY);
  gpio_reset_pin(KEY_R_PIN);
  gpio_set_direction(KEY_R_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(KEY_R_PIN, GPIO_PULLUP_ONLY);
  gpio_reset_pin(KEY_SELECT_PIN);
  gpio_set_direction(KEY_SELECT_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(KEY_SELECT_PIN, GPIO_PULLUP_ONLY);

  if (!display_host_init()) {
    return ESP_FAIL;
  }
  ESP_ERROR_CHECK(mount_sd_card());
  update_buttons();
  return ESP_OK;
}

extern "C" uint32_t mia_host_abi_version(void) { return 2; }

extern "C" void mia_host_log(const char *msg) {
  ESP_LOGI(TAG, "%s", msg != nullptr ? msg : "<null>");
}

extern "C" int32_t mia_host_screen_width(void) { return display_host_width(); }
extern "C" int32_t mia_host_screen_height(void) { return display_host_height(); }
extern "C" void mia_host_clear(uint8_t color) { display_host_clear(color); }
extern "C" void mia_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color) {
  display_host_fill_rect(x, y, w, h, color);
}
extern "C" void mia_host_fill_screen_rgb565(uint16_t color) { display_host_fill_screen_rgb565(color); }
extern "C" void mia_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg, uint8_t bg) {
  display_host_draw_text(x, y, text, fg, bg);
}
extern "C" void mia_host_present(void) { display_host_present(); }

extern "C" void mia_host_buttons_poll(void) { update_buttons(); }
extern "C" uint8_t mia_host_button_down(uint8_t b) { return b < 14 && g_buttons[b].down ? 1 : 0; }
extern "C" uint8_t mia_host_button_pressed(uint8_t b) { return b < 14 && g_buttons[b].pressed ? 1 : 0; }
extern "C" uint8_t mia_host_button_released(uint8_t b) { return b < 14 && g_buttons[b].released ? 1 : 0; }

extern "C" void mia_host_delay_ms(uint32_t ms) {
  if (ms == 0) {
    taskYIELD();
    return;
  }
  vTaskDelay(pdMS_TO_TICKS(ms));
}

extern "C" uint32_t mia_host_millis(void) {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

extern "C" void mia_host_backlight_set(uint8_t enabled) { display_host_backlight_set(enabled); }

extern "C" int32_t mia_host_sd_list_dir(const char *path, MiaHostDirEntry *entries, uint32_t capacity) {
  char vfs_path[256];
  DIR *dir;
  struct dirent *entry;
  uint32_t count = 0;

  if (!g_sd_ready || path == nullptr || entries == nullptr || capacity == 0) {
    return 0;
  }
  if (strcmp(path, "/") == 0) {
    snprintf(vfs_path, sizeof(vfs_path), "%s", ROOT_DIR);
  } else {
    snprintf(vfs_path, sizeof(vfs_path), "%s%s", ROOT_DIR, path);
  }
  dir = opendir(vfs_path);
  if (dir == nullptr) {
    return 0;
  }

  while (count < capacity && (entry = readdir(dir)) != nullptr) {
    char child_path[320];
    struct stat st = {};
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    if (snprintf(child_path, sizeof(child_path), "%s/%s", vfs_path, entry->d_name) >= (int)sizeof(child_path)) {
      continue;
    }
    if (stat(child_path, &st) != 0) {
      continue;
    }
    strncpy(entries[count].name, entry->d_name, sizeof(entries[count].name) - 1);
    entries[count].name[sizeof(entries[count].name) - 1] = '\0';
    entries[count].is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
    entries[count].size = S_ISDIR(st.st_mode) ? 0 : (uint32_t)st.st_size;
    ++count;
  }
  closedir(dir);
  return (int32_t)count;
}

extern "C" uint8_t mia_host_audio_open(uint32_t sample_rate, uint8_t channels, uint8_t bits_per_sample) {
  return audio_install(sample_rate, channels, bits_per_sample) ? 1 : 0;
}

extern "C" int32_t mia_host_audio_write_pcm16(const int16_t *samples, uint32_t frame_count, uint8_t channels) {
  uint32_t expanded = 0;
  const int16_t *output;
  size_t bytes_written = 0;
  size_t bytes;
  esp_err_t err;

  if (!g_audio.open || samples == nullptr || frame_count == 0) {
    return -1;
  }
  output = expand_frames(samples, frame_count, channels, &expanded);
  if (output == nullptr || expanded == 0) {
    g_audio.last_error = ESP_ERR_INVALID_ARG;
    return -1;
  }
  bytes = (size_t)expanded * 2u * sizeof(int16_t);
  err = i2s_write(I2S_NUM_0, output, bytes, &bytes_written, pdMS_TO_TICKS(250));
  if (err != ESP_OK) {
    g_audio.last_error = err;
    return -1;
  }
  g_audio.last_error = ESP_OK;
  return (int32_t)(bytes_written / (2u * sizeof(int16_t)));
}

extern "C" void mia_host_audio_stop(void) {
  if (g_audio.open) {
    i2s_zero_dma_buffer(I2S_NUM_0);
  }
}

extern "C" void mia_host_audio_close(void) {
  if (!g_audio.open) {
    return;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  i2s_driver_uninstall(I2S_NUM_0);
  gpio_set_level(AMP_CTRL_PIN, 0);
  memset(&g_audio, 0, sizeof(g_audio));
}

extern "C" uint8_t mia_host_audio_get_status(MiaHostAudioStatus *status) {
  if (status == nullptr) {
    return 0;
  }
  memset(status, 0, sizeof(*status));
  status->open = g_audio.open ? 1 : 0;
  status->sample_rate = g_audio.sample_rate;
  status->channels = g_audio.channels;
  status->bits_per_sample = g_audio.bits_per_sample;
  status->last_error = g_audio.last_error;
  return 1;
}

extern "C" uint8_t mia_host_rtc_read(MiaHostDateTime *) { return 0; }
extern "C" uint8_t mia_host_rtc_write(const MiaHostDateTime *) { return 0; }
extern "C" uint8_t mia_host_rtc_days_in_month(uint16_t, uint8_t) { return 0; }
extern "C" uint8_t mia_host_rtc_day_of_week(const MiaHostDateTime *) { return 0; }
extern "C" uint8_t mia_host_get_system_info(MiaHostSystemInfo *) { return 0; }
extern "C" uint8_t mia_host_read_battery(MiaHostBatteryInfo *) { return 0; }
extern "C" int32_t mia_host_wifi_scan(MiaHostWifiNetwork *, uint32_t) { return 0; }
extern "C" void mia_host_wifi_off(void) {}
extern "C" uint8_t mia_host_wifi_files_start(void) { return 0; }
extern "C" void mia_host_wifi_files_poll(void) {}
extern "C" void mia_host_wifi_files_stop(void) {}
extern "C" uint8_t mia_host_wifi_files_get_status(MiaHostWifiFilesStatus *) { return 0; }
extern "C" uint8_t mia_host_ftp_start(void) { return 0; }
extern "C" void mia_host_ftp_poll(void) {}
extern "C" void mia_host_ftp_stop(void) {}
extern "C" uint8_t mia_host_ftp_get_status(MiaHostFtpStatus *) { return 0; }
