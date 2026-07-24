#include "display_host.h"
#include "mia_host_abi.h"

#ifndef MIA_SD_MAX_FILES
#define MIA_SD_MAX_FILES 4
#endif

#pragma GCC diagnostic ignored "-Wformat-truncation"
#pragma GCC diagnostic ignored "-Wstringop-truncation"

#include <dirent.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/i2s.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_ota_ops.h>
#include <esp_rom_crc.h>
#include <esp_rom_sys.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_timer.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_partition.h>
#if __has_include(<esp_dlfcn.h>)
#include <esp_dlfcn.h>
#define MIA_HAS_ESP_DLOPEN 1
#endif
#include <driver/adc.h>
#include <esp_private/esp_clk.h>
#include <esp_vfs_fat.h>
#include <esp_wifi.h>
#include <soc/soc.h>
#include <ff.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <sdmmc_cmd.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if __has_include(<esp_http_server.h>)
#include <esp_http_server.h>
#define HAS_ESP_HTTP_SERVER 1
#endif

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

#ifndef MIA_AUDIO_DMA_BUF_COUNT
#define MIA_AUDIO_DMA_BUF_COUNT 8
#endif
#ifndef MIA_AUDIO_DMA_BUF_LEN
#define MIA_AUDIO_DMA_BUF_LEN 512
#endif

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
static int16_t g_stereo_scratch[2304 * 2] = {};

static void delay_us(uint32_t us) { esp_rom_delay_us(us); }

static uint8_t scan_hc165_once() {
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
  return val;
}

static void scan_hc165() {
  const uint8_t first = scan_hc165_once();
  const uint8_t second = scan_hc165_once();
  const uint8_t third = scan_hc165_once();
  g_hc165_state = (first & second) | (first & third) | (second & third);
}

static void update_buttons() {
  scan_hc165();
  for (size_t i = 0; i < 14; ++i) {
    bool down;
    if (BUTTONS[i].direct) {
      down = gpio_get_level(BUTTONS[i].gpio) == 0;
    } else {
#ifdef MIA_HC165_ACTIVE_HIGH
      down = (g_hc165_state & (1u << BUTTONS[i].shift_bit)) != 0;
#else
      down = (g_hc165_state & (1u << BUTTONS[i].shift_bit)) == 0;
#endif
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
  mount_cfg.max_files = MIA_SD_MAX_FILES;
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
  cfg.dma_buf_count = MIA_AUDIO_DMA_BUF_COUNT;
  cfg.dma_buf_len = MIA_AUDIO_DMA_BUF_LEN;
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
  if (channels != 1 || frame_count > 2304) {
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

namespace {

const char *WIFI_TAG = "host_wifi";

static bool s_wifi_init_done = false;
static bool s_wifi_started = false;

static esp_err_t wifi_ensure_init() {
  if (s_wifi_init_done) {
    return ESP_OK;
  }
  esp_err_t err;

  err = esp_netif_init();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    return err;
  }
  err = esp_event_loop_create_default();
  if (err != ESP_ERR_INVALID_STATE) {
    ESP_ERROR_CHECK(err);
  }

  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&cfg);
  if (err != ESP_OK) {
    return err;
  }
  s_wifi_init_done = true;
  return ESP_OK;
}

static void wifi_ensure_stop() {
  if (s_wifi_started) {
    esp_wifi_stop();
    s_wifi_started = false;
  }
}

static esp_err_t wifi_start_sta() {
  ESP_ERROR_CHECK(wifi_ensure_init());
  wifi_ensure_stop();
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  s_wifi_started = true;
  return ESP_OK;
}

static esp_err_t wifi_start_ap(const char *ssid, const char *password) {
  ESP_ERROR_CHECK(wifi_ensure_init());
  wifi_ensure_stop();
  wifi_config_t cfg = {};
  strncpy((char *)cfg.ap.ssid, ssid, sizeof(cfg.ap.ssid) - 1);
  if (password != nullptr && strlen(password) >= 8) {
    strncpy((char *)cfg.ap.password, password, sizeof(cfg.ap.password) - 1);
    cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
  } else {
    cfg.ap.authmode = WIFI_AUTH_OPEN;
  }
  cfg.ap.max_connection = 4;
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));
  ESP_ERROR_CHECK(esp_wifi_start());
  s_wifi_started = true;
  return ESP_OK;
}

static void wifi_get_ip(char *buf, size_t len, bool ap) {
  esp_netif_t *netif = ap ? esp_netif_get_handle_from_ifkey("WIFI_AP_DEF")
                          : esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif == nullptr) {
    snprintf(buf, len, "0.0.0.0");
    return;
  }
  esp_netif_ip_info_t ip;
  if (esp_netif_get_ip_info(netif, &ip) == ESP_OK) {
    snprintf(buf, len, IPSTR, IP2STR(&ip.ip));
  } else {
    snprintf(buf, len, "0.0.0.0");
  }
}

static bool load_wifi_config(char *ssid, size_t ssid_sz, char *password,
                             size_t pass_sz, char *ap_ssid, size_t ap_ssid_sz) {
  FILE *f = fopen("/sd/wifi.txt", "r");
  if (f == nullptr) {
    return false;
  }
  char line[128];
  while (fgets(line, sizeof(line), f)) {
    char *eq = strchr(line, '=');
    if (eq == nullptr) {
      continue;
    }
    *eq = '\0';
    const char *key = line;
    const char *val = eq + 1;

    size_t vlen = strlen(val);
    while (vlen > 0 && (val[vlen - 1] == '\n' || val[vlen - 1] == '\r')) {
      --vlen;
    }

    if (strcmp(key, "ssid") == 0 && vlen > 0) {
      size_t copy = vlen < ssid_sz - 1 ? vlen : ssid_sz - 1;
      memcpy(ssid, val, copy);
      ssid[copy] = '\0';
    } else if (strcmp(key, "password") == 0 && vlen > 0) {
      size_t copy = vlen < pass_sz - 1 ? vlen : pass_sz - 1;
      memcpy(password, val, copy);
      password[copy] = '\0';
    } else if (strcmp(key, "ap_ssid") == 0 && vlen > 0) {
      size_t copy = vlen < ap_ssid_sz - 1 ? vlen : ap_ssid_sz - 1;
      memcpy(ap_ssid, val, copy);
      ap_ssid[copy] = '\0';
    }
  }
  fclose(f);
  return true;
}

// --- RTC (PCF8563) helpers ---
constexpr gpio_num_t RTC_SCL_PIN = GPIO_NUM_4;
constexpr gpio_num_t RTC_SDA_PIN = GPIO_NUM_16;
constexpr uint8_t RTC_I2C_ADDR = 0x51;
constexpr uint8_t RTC_TIME_REG = 0x02;

static uint8_t toBcd(uint8_t v) {
  return static_cast<uint8_t>(((v / 10) << 4) | (v % 10));
}

static uint8_t fromBcd(uint8_t v) {
  return static_cast<uint8_t>(((v >> 4) * 10) + (v & 0x0F));
}

static bool isLeapYear(uint16_t y) {
  return (y % 400 == 0) || (y % 100 != 0 && y % 4 == 0);
}

static uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year)) return 29;
  return (month >= 1 && month <= 12) ? kDays[month - 1] : 0;
}

static uint8_t dayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  int y = year, m = month;
  if (m < 3) { m += 12; --y; }
  const int k = y % 100, j = y / 100;
  const int h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  return static_cast<uint8_t>((h + 6) % 7);
}

} // namespace

extern "C" esp_err_t host_platform_init(void) {
  // Initialize NVS before WiFi or any other component that needs it.
  esp_err_t nvs_err = nvs_flash_init();
  if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    const esp_err_t erase_err = nvs_flash_erase();
    if (erase_err == ESP_OK) {
      nvs_err = nvs_flash_init();
    }
  }
  if (nvs_err != ESP_OK && nvs_err != ESP_ERR_NVS_NOT_INITIALIZED) {
    ESP_LOGE(TAG, "nvs_flash_init failed: %d", nvs_err);
  }

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

  // --- I2C (PCF8563 RTC + expansion) ---
  i2c_config_t i2c_conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = RTC_SDA_PIN,
      .scl_io_num = RTC_SCL_PIN,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master = {.clk_speed = 100000},
      .clk_flags = 0,
  };
  esp_err_t i2c_err = i2c_param_config(I2C_NUM_0, &i2c_conf);
  if (i2c_err == ESP_OK) {
    i2c_err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
  }
  if (i2c_err != ESP_OK) {
    ESP_LOGE(TAG, "I2C init failed: %d", i2c_err);
  }

#ifdef MIA_DISPLAY_DROID_GBK_SHARED
  ESP_ERROR_CHECK(mount_sd_card());
#endif
  if (!display_host_init()) {
    return ESP_FAIL;
  }
#ifndef MIA_DISPLAY_DROID_GBK_SHARED
  ESP_ERROR_CHECK(mount_sd_card());
#endif
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
extern "C" int32_t mia_host_text_height(void) { return display_host_text_height(); }
extern "C" int32_t mia_host_text_width(const char *text) {
  return display_host_text_width(text);
}
extern "C" void mia_host_present(void) { display_host_present(); }
extern "C" int32_t mia_host_present_rgb565(const uint16_t *pixels, uint32_t width,
                                             uint32_t height, uint32_t pitch_bytes) {
  return display_host_present_rgb565(pixels, width, height, pitch_bytes);
}

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

extern "C" uint8_t mia_host_language(void) {
  nvs_handle_t language_store;
  if (nvs_open("mia-i18n", NVS_READONLY, &language_store) != ESP_OK) return 0;
  uint8_t language = 0;
  if (nvs_get_u8(language_store, "lang", &language) != ESP_OK) language = 0;
  nvs_close(language_store);
  return language == 1 ? 1 : 0;
}

extern "C" void mia_host_backlight_set(uint8_t enabled) { display_host_backlight_set(enabled); }

extern "C" int32_t mia_host_sd_list_dir(const char *path, MiaHostDirEntry *entries, uint32_t capacity) {
  char vfs_path[1024];
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
    char child_path[1280];
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

#ifdef MIA_SD_BROWSER_FEATURES
extern "C" uint8_t sd_browser_host_language(void) {
  return mia_host_language();
}

extern "C" int32_t sd_browser_host_stat(const char *path, uint32_t *size,
                                          int64_t *modified) {
  if (!g_sd_ready || path == nullptr || size == nullptr || modified == nullptr) {
    return -1;
  }
  char vfs_path[1280];
  const int written = strcmp(path, "/") == 0
                          ? snprintf(vfs_path, sizeof(vfs_path), "%s", ROOT_DIR)
                          : snprintf(vfs_path, sizeof(vfs_path), "%s%s", ROOT_DIR, path);
  if (written < 0 || written >= static_cast<int>(sizeof(vfs_path))) {
    return -1;
  }
  struct stat info = {};
  if (stat(vfs_path, &info) != 0) {
    return -1;
  }
  *size = S_ISDIR(info.st_mode) ? 0 : static_cast<uint32_t>(info.st_size);
  *modified = static_cast<int64_t>(info.st_mtime);
  return 0;
}

extern "C" int32_t sd_browser_host_capacity(uint64_t *free_bytes,
                                              uint64_t *total_bytes) {
  if (!g_sd_ready || free_bytes == nullptr || total_bytes == nullptr) {
    return -1;
  }
  FATFS *fs = nullptr;
  DWORD free_clusters = 0;
  if (f_getfree("0:", &free_clusters, &fs) != FR_OK || fs == nullptr) {
    return -1;
  }
  const uint64_t bytes_per_cluster = static_cast<uint64_t>(fs->csize) * fs->ssize;
  *total_bytes = static_cast<uint64_t>(fs->n_fatent - 2) * bytes_per_cluster;
  *free_bytes = static_cast<uint64_t>(free_clusters) * bytes_per_cluster;
  return 0;
}
#endif

extern "C" int32_t mia_host_sd_remove(const char *path) {
  if (!g_sd_ready || path == nullptr) {
    return -1;
  }
  char vfs_path[1280];
  if (strcmp(path, "/") == 0) {
    return -1;
  }
  snprintf(vfs_path, sizeof(vfs_path), "%s%s", ROOT_DIR, path);

  struct stat st;
  if (stat(vfs_path, &st) != 0) {
    return -1;
  }

  if (S_ISDIR(st.st_mode)) {
    return rmdir(vfs_path);
  }
  return unlink(vfs_path);
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

/* Write bytes to the RTC starting at the given register. */
static bool rtcWriteRegs(uint8_t reg, const uint8_t *data, size_t len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);
  for (size_t i = 0; i < len; ++i) {
    i2c_master_write_byte(cmd, data[i], true);
  }
  i2c_master_stop(cmd);
  esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);
  return err == ESP_OK;
}

/* Read bytes from the RTC starting at the given register. */
static bool rtcReadRegs(uint8_t reg, uint8_t *buf, size_t len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  /* Write register address (with repeated start) */
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);
  /* Repeated start + read */
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (RTC_I2C_ADDR << 1) | I2C_MASTER_READ, true);
  if (len > 1) {
    i2c_master_read(cmd, buf, len - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, &buf[len - 1], I2C_MASTER_NACK);
  i2c_master_stop(cmd);
  esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);
  return err == ESP_OK;
}

extern "C" uint8_t mia_host_rtc_read(MiaHostDateTime *dt) {
  if (dt == nullptr) return 0;
  uint8_t buf[7];
  if (!rtcReadRegs(RTC_TIME_REG, buf, 7)) return 0;

  dt->second  = fromBcd(buf[0] & 0x7F);
  dt->minute  = fromBcd(buf[1] & 0x7F);
  dt->hour    = fromBcd(buf[2] & 0x3F);
  dt->day     = fromBcd(buf[3] & 0x3F);
  dt->weekday = buf[4] & 0x07;
  dt->month   = fromBcd(buf[5] & 0x1F);
  dt->year    = static_cast<uint16_t>(2000 + fromBcd(buf[6]));
  if (dt->month < 1) dt->month = 1;
  if (dt->month > 12) dt->month = 12;
  const uint8_t maxDay = daysInMonth(dt->year, dt->month);
  if (dt->day < 1) dt->day = 1;
  if (dt->day > maxDay) dt->day = maxDay;
  if (dt->weekday > 6) dt->weekday = dayOfWeek(dt->year, dt->month, dt->day);
  return 1;
}

extern "C" uint8_t mia_host_rtc_write(const MiaHostDateTime *dt) {
  if (dt == nullptr) return 0;
  uint8_t wday = dayOfWeek(dt->year, dt->month, dt->day);
  /* Clear the PCF8563 control/status so the clock runs and alarms are off. */
  const uint8_t ctrl[2] = {0x00, 0x00};
  rtcWriteRegs(0x00, ctrl, 2);
  /* Write the 7 time registers starting at 0x02. VL bit (bit7 of seconds) = 0. */
  uint8_t data[7];
  data[0] = toBcd(dt->second);
  data[1] = toBcd(dt->minute);
  data[2] = toBcd(dt->hour);
  data[3] = toBcd(dt->day);
  data[4] = static_cast<uint8_t>(toBcd(wday) & 0x07);
  data[5] = static_cast<uint8_t>(toBcd(dt->month) & 0x1F);
  data[6] = toBcd(static_cast<uint8_t>(dt->year % 100));
  bool ok = rtcWriteRegs(RTC_TIME_REG, data, 7);
  return ok ? 1 : 0;
}

extern "C" uint8_t mia_host_rtc_days_in_month(uint16_t year, uint8_t month) {
  return daysInMonth(year, month);
}

extern "C" uint8_t mia_host_rtc_day_of_week(const MiaHostDateTime *dt) {
  if (dt == nullptr) return 0;
  return dayOfWeek(dt->year, dt->month, dt->day);
}
extern "C" uint8_t mia_host_get_system_info(MiaHostSystemInfo *info) {
  if (info == nullptr) return 0;
  memset(info, 0, sizeof(MiaHostSystemInfo));

  esp_chip_info_t chip;
  esp_chip_info(&chip);

  switch (chip.model) {
    case CHIP_ESP32:    strncpy(info->chip_model, "ESP32",    sizeof(info->chip_model) - 1); break;
    case CHIP_ESP32S2:  strncpy(info->chip_model, "ESP32-S2", sizeof(info->chip_model) - 1); break;
    case CHIP_ESP32S3:  strncpy(info->chip_model, "ESP32-S3", sizeof(info->chip_model) - 1); break;
#ifdef CONFIG_IDF_TARGET_ESP32C2
    case CHIP_ESP32C2:  strncpy(info->chip_model, "ESP32-C2", sizeof(info->chip_model) - 1); break;
#endif
    case CHIP_ESP32C3:  strncpy(info->chip_model, "ESP32-C3", sizeof(info->chip_model) - 1); break;
#ifdef CONFIG_IDF_TARGET_ESP32C6
    case CHIP_ESP32C6:  strncpy(info->chip_model, "ESP32-C6", sizeof(info->chip_model) - 1); break;
#endif
    case CHIP_ESP32H2:  strncpy(info->chip_model, "ESP32-H2", sizeof(info->chip_model) - 1); break;
    default:            strncpy(info->chip_model, "ESP32",    sizeof(info->chip_model) - 1); break;
  }

  info->chip_revision = (uint8_t)chip.revision;
  info->cpu_mhz = (uint16_t)(esp_clk_cpu_freq() / 1000000);
  info->free_heap_kb = esp_get_free_heap_size() / 1024;

  uint32_t flash_size = 0;
  if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
    info->flash_mb = flash_size / (1024 * 1024);
  }

  info->tft_ready = display_host_ready() ? 1 : 0;
  info->sd_ready = g_sd_ready ? 1 : 0;
  return 1;
}

extern "C" uint8_t mia_host_read_battery(MiaHostBatteryInfo *info) {
  if (info == nullptr) return 0;
  memset(info, 0, sizeof(MiaHostBatteryInfo));

  // ADC1_CH0 = GPIO1 (VBAT_VOLTAGE_PIN)
  static bool adc_initialized = false;
  if (!adc_initialized) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc_initialized = true;
  }

  int raw = adc1_get_raw(ADC1_CHANNEL_0);
  if (raw < 0) raw = 0;

  info->raw = raw;
  // VBAT = raw * 3.3V / 4095 * 2.0 (VBAT_DIVIDER)
  float volts = (float)raw * 3.3f / 4095.0f * 2.0f;
  info->millivolts = (uint32_t)(volts * 1000.0f);
  return 1;
}

// ---------------------------------------------------------------------------
// WiFi scan ABI
// ---------------------------------------------------------------------------
extern "C" int32_t mia_host_wifi_scan(MiaHostWifiNetwork *networks,
                                      uint32_t capacity) {
  if (networks == nullptr || capacity == 0) {
    return -1;
  }
  if (wifi_start_sta() != ESP_OK) {
    return -1;
  }

  // Blocking scan
  wifi_scan_config_t scan_cfg = {};
  scan_cfg.show_hidden = false;
  scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  esp_err_t err = esp_wifi_scan_start(&scan_cfg, true);
  if (err != ESP_OK) {
    wifi_ensure_stop();
    return -1;
  }

  uint16_t found = 0;
  err = esp_wifi_scan_get_ap_num(&found);
  if (err != ESP_OK) {
    wifi_ensure_stop();
    return -1;
  }

  uint16_t count = (found < capacity) ? found : (uint16_t)capacity;
  wifi_ap_record_t *records = (wifi_ap_record_t *)malloc(
      (size_t)count * sizeof(wifi_ap_record_t));
  if (records == nullptr) {
    wifi_ensure_stop();
    return -1;
  }

  err = esp_wifi_scan_get_ap_records(&count, records);
  if (err != ESP_OK) {
    free(records);
    wifi_ensure_stop();
    return -1;
  }

  for (uint16_t i = 0; i < count; ++i) {
    strncpy(networks[i].ssid, (const char *)records[i].ssid,
            sizeof(networks[i].ssid) - 1);
    networks[i].ssid[sizeof(networks[i].ssid) - 1] = '\0';
    networks[i].rssi = (int32_t)records[i].rssi;
  }
  free(records);

  // Keep WiFi running for possible rescan
  return (int32_t)count;
}

extern "C" void mia_host_wifi_off(void) {
  wifi_ensure_stop();
  if (s_wifi_init_done) {
    esp_wifi_deinit();
    s_wifi_init_done = false;
  }
}

// ---------------------------------------------------------------------------
// WiFi Files (HTTP server) ABI
// ---------------------------------------------------------------------------
#ifdef HAS_ESP_HTTP_SERVER

namespace {

const char *WF_TAG = "wifi_files";
static bool s_wf_running = false;
static bool s_wf_ap_mode = false;
static char s_wf_status[32] = "Stopped";
static char s_wf_ssid[32] = {};
static char s_wf_ip[16] = {};
static httpd_handle_t s_wf_server = nullptr;

static const char *wf_mime_type(const char *path) {
  const char *ext = strrchr(path, '.');
  if (ext == nullptr) { return "application/octet-stream"; }
  if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0)
    return "text/html";
  if (strcasecmp(ext, ".txt") == 0) return "text/plain";
  if (strcasecmp(ext, ".css") == 0) return "text/css";
  if (strcasecmp(ext, ".js") == 0) return "application/javascript";
  if (strcasecmp(ext, ".png") == 0) return "image/png";
  if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
    return "image/jpeg";
  if (strcasecmp(ext, ".gif") == 0) return "image/gif";
  if (strcasecmp(ext, ".json") == 0) return "application/json";
  if (strcasecmp(ext, ".bin") == 0 || strcasecmp(ext, ".elf") == 0)
    return "application/octet-stream";
  return "application/octet-stream";
}

// Build HTML directory listing
static char *wf_build_listing(const char *vfs_path, const char *url_path) {
  DIR *dir = opendir(vfs_path);
  if (dir == nullptr) { return nullptr; }

  // First pass: count entries
  struct dirent *entry;
  int count = 0;
  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    ++count;
  }
  rewinddir(dir);

  // Allocate a generous buffer
  size_t buf_sz = 4096 + (size_t)count * 256;
  char *html = (char *)malloc(buf_sz);
  if (html == nullptr) {
    closedir(dir);
    return nullptr;
  }

  size_t pos = 0;
  int n = snprintf(html + pos, buf_sz - pos,
      "<!DOCTYPE html><html><head>"
      "<meta charset=utf-8>"
      "<meta name=viewport content='width=device-width,initial-scale=1'>"
      "<title>%s</title>"
      "<style>"
      "body{font-family:sans-serif;margin:20px;background:#111;color:#eee}"
      "a{color:#4cf}"
      "td{padding:4px 16px}"
      "input,button{font-size:16px}"
      "</style></head><body>"
      "<h2>Index of %s</h2><table>",
      url_path, url_path);
  if (n > 0) pos += (size_t)n;

  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char child[320];
    struct stat st;
    snprintf(child, sizeof(child), "%s/%s", vfs_path, entry->d_name);
    bool is_dir = (stat(child, &st) == 0 && S_ISDIR(st.st_mode));

    // URL-encode simple html-unsafe chars in the name
    char safe_name[256];
    const char *s = entry->d_name;
    char *d = safe_name;
    while (*s && (size_t)(d - safe_name) < sizeof(safe_name) - 6) {
      if (*s == '<' || *s == '>' || *s == '&' || *s == '"' || *s == '\'') {
        d += snprintf(d, 6, "%%%02X", (unsigned char)*s);
      } else {
        *d++ = *s;
      }
      ++s;
    }
    *d = '\0';

    char url[320];
    if (strcmp(url_path, "/") == 0) {
      snprintf(url, sizeof(url), "/%s%s", safe_name,
               is_dir ? "/" : "");
    } else {
      snprintf(url, sizeof(url), "%s/%s%s", url_path, safe_name,
               is_dir ? "/" : "");
    }

    char size_str[32] = "";
    if (!is_dir) {
      if (st.st_size < 1024) {
        snprintf(size_str, sizeof(size_str), "%" PRIu32, (uint32_t)st.st_size);
      } else if (st.st_size < 1024 * 1024) {
        snprintf(size_str, sizeof(size_str), "%" PRIu32 "KB",
                 (uint32_t)(st.st_size / 1024));
      } else {
        snprintf(size_str, sizeof(size_str), "%" PRIu32 "MB",
                 (uint32_t)(st.st_size / (1024 * 1024)));
      }
    }

    n = snprintf(html + pos, buf_sz - pos,
                 "<tr><td><a href=\"%s\">%s%s</a></td>"
                 "<td>%s</td></tr>",
                 url, safe_name, is_dir ? "/" : "", size_str);
    if (n > 0) pos += (size_t)n;
  }
  closedir(dir);

  n = snprintf(html + pos, buf_sz - pos,
      "</table><hr>"
      "<h3>Upload</h3>"
      "<form action=\"/upload\" method=post enctype=multipart/form-data>"
      "<input type=file name=file><br>"
      "<input type=submit value=Upload>"
      "</form>"
      "<br><a href=\"/\">Root</a>"
      "</body></html>");
  if (n > 0) pos += (size_t)n;

  return html;
}

// GET / or /path — directory listing
static esp_err_t wf_root_handler(httpd_req_t *req) {
  const char *url = req->uri;
  char vfs_path[256];

  if (strcmp(url, "/") == 0) {
    snprintf(vfs_path, sizeof(vfs_path), "/sd");
  } else {
    snprintf(vfs_path, sizeof(vfs_path), "/sd%s", url);
  }

  struct stat st;
  if (stat(vfs_path, &st) != 0) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_FAIL;
  }

  if (S_ISDIR(st.st_mode)) {
    char *html = wf_build_listing(vfs_path, url);
    if (html == nullptr) {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
      return ESP_FAIL;
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    free(html);
    return ESP_OK;
  }

  // Serve file
  FILE *f = fopen(vfs_path, "rb");
  if (f == nullptr) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, NULL);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, wf_mime_type(vfs_path));
  char buf[1024];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    if (httpd_resp_send_chunk(req, buf, n) != ESP_OK) {
      break;
    }
  }
  fclose(f);
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

// POST /upload — handle file upload
static esp_err_t wf_upload_handler(httpd_req_t *req) {
  if (req->method != HTTP_POST) {
    return ESP_FAIL;
  }

  // Extract filename from content-disposition
  char content[1024];
  int ret = httpd_req_recv(req, content, sizeof(content) - 1);
  if (ret <= 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    return ESP_FAIL;
  }
  content[ret] = '\0';

  // Simple multipart parser — find filename= and then body after boundary
  char filename[128] = "uploaded";
  const char *fn_start = strstr(content, "filename=\"");
  if (fn_start) {
    fn_start += 10;
    const char *fn_end = strchr(fn_start, '"');
    if (fn_end) {
      size_t fn_len = (size_t)(fn_end - fn_start);
      if (fn_len > 0 && fn_len < sizeof(filename)) {
        memcpy(filename, fn_start, fn_len);
        filename[fn_len] = '\0';
      }
    }
  }

  // Build SD path — save to root by default
  char sd_path[256];
  snprintf(sd_path, sizeof(sd_path), "/sd/%s", filename);

  FILE *f = fopen(sd_path, "wb");
  if (f == nullptr) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
    return ESP_FAIL;
  }

  // Find the file data start (after the second \r\n\r\n)
  const char *data = strstr(content, "\r\n\r\n");
  if (data) {
    data += 4;
    size_t data_len = (size_t)(content + ret - data);
    // Strip trailing boundary
    const char *boundary = strstr(data, "\r\n------");
    if (boundary) {
      data_len = (size_t)(boundary - data);
    }
    fwrite(data, 1, data_len, f);
  }

  // Read any remaining chunks
  char buf[1024];
  int n;
  while ((n = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
    fwrite(buf, 1, (size_t)n, f);
  }
  fclose(f);

  // Redirect back
  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

// POST /delete
static esp_err_t wf_delete_handler(httpd_req_t *req) {
  char buf[256];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  // Expect: path=<urlencoded path>
  const char *p = strstr(buf, "path=");
  if (p == nullptr) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    return ESP_FAIL;
  }
  p += 5;

  char sd_path[256];
  snprintf(sd_path, sizeof(sd_path), "/sd/%s", p);
  struct stat st;
  if (stat(sd_path, &st) != 0) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, NULL);
    return ESP_FAIL;
  }

  if (S_ISDIR(st.st_mode)) {
    rmdir(sd_path);
  } else {
    unlink(sd_path);
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

// POST /mkdir
static esp_err_t wf_mkdir_handler(httpd_req_t *req) {
  char buf[256];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  const char *p = strstr(buf, "path=");
  if (p == nullptr) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    return ESP_FAIL;
  }
  p += 5;

  char sd_path[256];
  snprintf(sd_path, sizeof(sd_path), "/sd/%s", p);
  mkdir(sd_path, 0755);

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

static const httpd_uri_t wf_handlers[] = {
    {"/", HTTP_GET, wf_root_handler, NULL},
    {"/*", HTTP_GET, wf_root_handler, NULL},
    {"/upload", HTTP_POST, wf_upload_handler, NULL},
    {"/delete", HTTP_POST, wf_delete_handler, NULL},
    {"/mkdir", HTTP_POST, wf_mkdir_handler, NULL},
};

} // namespace

extern "C" uint8_t mia_host_wifi_files_start(void) {
  if (s_wf_running) {
    return 1;
  }

  strcpy(s_wf_status, "Starting...");

  char station_ssid[64] = {};
  char station_pass[64] = {};
  char ap_ssid[32] = {};
  load_wifi_config(station_ssid, sizeof(station_ssid), station_pass,
                   sizeof(station_pass), ap_ssid, sizeof(ap_ssid));

  if (ap_ssid[0] == '\0') {
    strcpy(ap_ssid, "MiaOS");
  }

  bool connected = false;
  if (station_ssid[0] != '\0') {
    if (wifi_start_sta() == ESP_OK) {
      wifi_config_t sta_cfg = {};
      strncpy((char *)sta_cfg.sta.ssid, station_ssid,
              sizeof(sta_cfg.sta.ssid) - 1);
      strncpy((char *)sta_cfg.sta.password, station_pass,
              sizeof(sta_cfg.sta.password) - 1);
      esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
      esp_wifi_connect();

      // Wait for connection
      for (int i = 0; i < 60; ++i) {
        vTaskDelay(pdMS_TO_TICKS(200));
        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
          connected = true;
          break;
        }
      }
    }
  }

  if (connected) {
    s_wf_ap_mode = false;
    strcpy(s_wf_ssid, station_ssid);
    ESP_LOGI(WF_TAG, "connected to STA %s", station_ssid);
  } else {
    // Fallback to AP
    s_wf_ap_mode = true;
    strcpy(s_wf_ssid, ap_ssid);
    ESP_LOGI(WF_TAG, "starting AP %s", ap_ssid);
    if (wifi_start_ap(ap_ssid, nullptr) != ESP_OK) {
      strcpy(s_wf_status, "WiFi fail");
      return 0;
    }
  }

  wifi_get_ip(s_wf_ip, sizeof(s_wf_ip), s_wf_ap_mode);

  // Start HTTP server
  httpd_config_t httpd_cfg = HTTPD_DEFAULT_CONFIG();
  httpd_cfg.max_uri_handlers = 8;
  httpd_cfg.max_open_sockets = 2;
  httpd_cfg.lru_purge_enable = true;
  if (httpd_start(&s_wf_server, &httpd_cfg) != ESP_OK) {
    strcpy(s_wf_status, "HTTP fail");
    wifi_ensure_stop();
    return 0;
  }

  for (size_t i = 0; i < sizeof(wf_handlers) / sizeof(wf_handlers[0]); ++i) {
    httpd_register_uri_handler(s_wf_server, &wf_handlers[i]);
  }

  s_wf_running = true;
  strcpy(s_wf_status, "HTTP ready");
  ESP_LOGI(WF_TAG, "HTTP server http://%s/", s_wf_ip);
  return 1;
}

extern "C" void mia_host_wifi_files_poll(void) {
  // httpd runs in its own task; nothing to do here
}

extern "C" void mia_host_wifi_files_stop(void) {
  if (s_wf_server) {
    httpd_stop(s_wf_server);
    s_wf_server = nullptr;
  }
  s_wf_running = false;
  wifi_ensure_stop();
  strcpy(s_wf_status, "Stopped");
  ESP_LOGI(WF_TAG, "stopped");
}

extern "C" uint8_t mia_host_wifi_files_get_status(MiaHostWifiFilesStatus *status) {
  if (status == nullptr) {
    return 0;
  }
  memset(status, 0, sizeof(*status));
  status->running = s_wf_running ? 1 : 0;
  status->ap_mode = s_wf_ap_mode ? 1 : 0;
  strncpy(status->status, s_wf_status, sizeof(status->status) - 1);
  strncpy(status->ssid, s_wf_ssid, sizeof(status->ssid) - 1);
  strncpy(status->ip, s_wf_ip, sizeof(status->ip) - 1);
  return 1;
}

#else  // !HAS_ESP_HTTP_SERVER

extern "C" uint8_t mia_host_wifi_files_start(void) { return 0; }
extern "C" void mia_host_wifi_files_poll(void) {}
extern "C" void mia_host_wifi_files_stop(void) {}
extern "C" uint8_t mia_host_wifi_files_get_status(MiaHostWifiFilesStatus *) { return 0; }

#endif // HAS_ESP_HTTP_SERVER

// ---------------------------------------------------------------------------
// FTP server ABI
// ---------------------------------------------------------------------------
namespace {

const char *FTP_TAG = "ftp_server";

static bool s_ftp_running = false;
static char s_ftp_status[32] = "Stopped";
static char s_ftp_ip[16] = {};
static int s_ftp_ctrl_sock = -1;
static int s_ftp_data_sock = -1;
static int s_ftp_client_sock = -1;
static int s_ftp_data_listen_sock = -1;

// FTP session state
static char s_ftp_user[32] = {};
static bool s_ftp_authenticated = false;
static char s_ftp_cwd[256] = "/";
static int s_ftp_data_port = 0;
static bool s_ftp_pasv_mode = false;

static void ftp_send(int fd, const char *msg) {
  if (fd < 0) { return; }
  size_t len = strlen(msg);
  send(fd, msg, len, 0);
}

static void ftp_sendf(int fd, const char *fmt, ...) {
  if (fd < 0) { return; }
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  size_t len = strlen(buf);
  send(fd, buf, len, 0);
}

static int ftp_recv_line(int fd, char *buf, size_t sz) {
  size_t pos = 0;
  while (pos < sz - 1) {
    char c;
    int n = recv(fd, &c, 1, 0);
    if (n <= 0) { return -1; }
    if (c == '\n') { break; }
    if (c != '\r') { buf[pos++] = c; }
  }
  buf[pos] = '\0';
  return (int)pos;
}

static void ftp_handle_client(int fd) {
  char line[256];
  char cmd[16];
  char arg[240];

  ftp_send(fd, "220 MiaOS FTP ready\r\n");

  while (1) {
    memset(line, 0, sizeof(line));
    memset(cmd, 0, sizeof(cmd));
    memset(arg, 0, sizeof(arg));

    if (ftp_recv_line(fd, line, sizeof(line)) < 0) {
      break;
    }

    // Parse command and argument
    char *space = strchr(line, ' ');
    if (space) {
      *space = '\0';
      strncpy(cmd, line, sizeof(cmd) - 1);
      strncpy(arg, space + 1, sizeof(arg) - 1);
    } else {
      strncpy(cmd, line, sizeof(cmd) - 1);
    }

    // Convert command to uppercase for comparison
    for (char *p = cmd; *p; ++p) {
      if (*p >= 'a' && *p <= 'z') { *p -= 32; }
    }

    if (strcmp(cmd, "QUIT") == 0) {
      ftp_send(fd, "221 Bye\r\n");
      break;
    } else if (strcmp(cmd, "USER") == 0) {
      strncpy(s_ftp_user, arg, sizeof(s_ftp_user) - 1);
      ftp_send(fd, "230 Guest login OK\r\n");
      s_ftp_authenticated = true;
    } else if (strcmp(cmd, "PASS") == 0) {
      ftp_send(fd, "230 Already authenticated\r\n");
      s_ftp_authenticated = true;
    } else if (strcmp(cmd, "SYST") == 0) {
      ftp_send(fd, "215 UNIX Type: L8\r\n");
    } else if (strcmp(cmd, "PWD") == 0) {
      ftp_sendf(fd, "257 \"%s\"\r\n", s_ftp_cwd);
    } else if (strcmp(cmd, "TYPE") == 0) {
      ftp_send(fd, "200 Type set\r\n");
    } else if (strcmp(cmd, "PASV") == 0) {
      // Open data listen socket
      if (s_ftp_data_listen_sock >= 0) {
        close(s_ftp_data_listen_sock);
      }

      s_ftp_data_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
      if (s_ftp_data_listen_sock < 0) {
        ftp_send(fd, "425 Can't open data port\r\n");
        continue;
      }

      struct sockaddr_in addr;
      int opt = 1;
      setsockopt(s_ftp_data_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port = 0;
      if (bind(s_ftp_data_listen_sock, (struct sockaddr *)&addr,
               sizeof(addr)) < 0) {
        close(s_ftp_data_listen_sock);
        s_ftp_data_listen_sock = -1;
        ftp_send(fd, "425 Can't open data port\r\n");
        continue;
      }

      socklen_t addr_len = sizeof(addr);
      getsockname(s_ftp_data_listen_sock, (struct sockaddr *)&addr, &addr_len);
      s_ftp_data_port = ntohs(addr.sin_port);

      listen(s_ftp_data_listen_sock, 1);

      // Get our IP for PASV response
      struct sockaddr_in local_addr;
      socklen_t local_len = sizeof(local_addr);
      char pasv_ip[16] = "0,0,0,0";
      if (getsockname(fd, (struct sockaddr *)&local_addr, &local_len) == 0) {
        uint32_t ip = ntohl(local_addr.sin_addr.s_addr);
        snprintf(pasv_ip, sizeof(pasv_ip), "%lu,%lu,%lu,%lu",
                 (unsigned long)((ip >> 24) & 0xFF),
                 (unsigned long)((ip >> 16) & 0xFF),
                 (unsigned long)((ip >> 8) & 0xFF),
                 (unsigned long)(ip & 0xFF));
      }

      ftp_sendf(fd, "227 Entering Passive Mode (%s,%lu,%lu)\r\n", pasv_ip,
                (unsigned long)((s_ftp_data_port >> 8) & 0xFF),
                (unsigned long)(s_ftp_data_port & 0xFF));
      s_ftp_pasv_mode = true;

    } else if (strcmp(cmd, "EPSV") == 0) {
      // Extended passive mode — use same port as PASV
      if (s_ftp_data_listen_sock >= 0) {
        close(s_ftp_data_listen_sock);
      }
      s_ftp_data_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
      if (s_ftp_data_listen_sock < 0) {
        ftp_send(fd, "425 Can't open data port\r\n");
        continue;
      }
      struct sockaddr_in addr;
      int opt = 1;
      setsockopt(s_ftp_data_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port = 0;
      if (bind(s_ftp_data_listen_sock, (struct sockaddr *)&addr,
               sizeof(addr)) < 0) {
        close(s_ftp_data_listen_sock);
        s_ftp_data_listen_sock = -1;
        ftp_send(fd, "425 Can't open data port\r\n");
        continue;
      }
      socklen_t addr_len = sizeof(addr);
      getsockname(s_ftp_data_listen_sock, (struct sockaddr *)&addr, &addr_len);
      listen(s_ftp_data_listen_sock, 1);
      s_ftp_pasv_mode = true;
      ftp_sendf(fd, "229 Entering Extended Passive Mode (|||%u|)\r\n",
                ntohs(addr.sin_port));

    } else if (strcmp(cmd, "PORT") == 0) {
      // PORT h1,h2,h3,h4,p1,p2 — not supported, require PASV
      ftp_send(fd, "500 Use PASV\r\n");

    } else if (strcmp(cmd, "LIST") == 0) {
      // Accept data connection
      int data_fd = -1;
      if (s_ftp_pasv_mode && s_ftp_data_listen_sock >= 0) {
        struct sockaddr_in data_addr;
        socklen_t data_len = sizeof(data_addr);
        data_fd = accept(s_ftp_data_listen_sock,
                         (struct sockaddr *)&data_addr, &data_len);
      }
      if (data_fd < 0) {
        ftp_send(fd, "425 No data connection\r\n");
        continue;
      }

      ftp_send(fd, "150 Listing\r\n");

      // Build path
      char path[256];
      if (arg[0] == '\0') {
        snprintf(path, sizeof(path), "/sd%s", s_ftp_cwd);
      } else if (arg[0] == '/') {
        snprintf(path, sizeof(path), "/sd%s", arg);
      } else {
        snprintf(path, sizeof(path), "/sd%s/%s", s_ftp_cwd, arg);
      }

      DIR *dir = opendir(path);
      if (dir) {
        struct dirent *entry;
        char listing[1024];
        size_t list_pos = 0;
        while ((entry = readdir(dir)) != nullptr &&
               list_pos < sizeof(listing) - 128) {
          if (strcmp(entry->d_name, ".") == 0 ||
              strcmp(entry->d_name, "..") == 0)
            continue;
          int n = snprintf(listing + list_pos, sizeof(listing) - list_pos,
                           "drwxr-xr-x 1 0 0 0 Jan 1 00:00 %s\r\n",
                           entry->d_name);
          if (n > 0) { list_pos += (size_t)n; }
        }
        closedir(dir);
        send(data_fd, listing, list_pos, 0);
      }
      close(data_fd);
      if (s_ftp_data_listen_sock >= 0) {
        close(s_ftp_data_listen_sock);
        s_ftp_data_listen_sock = -1;
      }
      s_ftp_pasv_mode = false;
      ftp_send(fd, "226 Listing done\r\n");

    } else if (strcmp(cmd, "RETR") == 0) {
      if (arg[0] == '\0') {
        ftp_send(fd, "501 Syntax error\r\n");
        continue;
      }
      char path[256];
      if (arg[0] == '/') {
        snprintf(path, sizeof(path), "/sd%s", arg);
      } else {
        snprintf(path, sizeof(path), "/sd%s/%s", s_ftp_cwd, arg);
      }

      FILE *f = fopen(path, "rb");
      if (f == nullptr) {
        ftp_send(fd, "550 File not found\r\n");
        continue;
      }

      int data_fd = -1;
      if (s_ftp_pasv_mode && s_ftp_data_listen_sock >= 0) {
        struct sockaddr_in data_addr;
        socklen_t data_len = sizeof(data_addr);
        data_fd = accept(s_ftp_data_listen_sock,
                         (struct sockaddr *)&data_addr, &data_len);
      }
      if (data_fd < 0) {
        fclose(f);
        ftp_send(fd, "425 No data connection\r\n");
        continue;
      }

      ftp_send(fd, "150 Sending file\r\n");
      char buf[1024];
      size_t n;
      while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        send(data_fd, buf, n, 0);
      }
      fclose(f);
      close(data_fd);
      if (s_ftp_data_listen_sock >= 0) {
        close(s_ftp_data_listen_sock);
        s_ftp_data_listen_sock = -1;
      }
      s_ftp_pasv_mode = false;
      ftp_send(fd, "226 Transfer complete\r\n");

    } else if (strcmp(cmd, "STOR") == 0) {
      if (arg[0] == '\0') {
        ftp_send(fd, "501 Syntax error\r\n");
        continue;
      }
      char path[256];
      if (arg[0] == '/') {
        snprintf(path, sizeof(path), "/sd%s", arg);
      } else {
        snprintf(path, sizeof(path), "/sd%s/%s", s_ftp_cwd, arg);
      }

      FILE *f = fopen(path, "wb");
      if (f == nullptr) {
        ftp_send(fd, "550 Can't create file\r\n");
        continue;
      }

      int data_fd = -1;
      if (s_ftp_pasv_mode && s_ftp_data_listen_sock >= 0) {
        struct sockaddr_in data_addr;
        socklen_t data_len = sizeof(data_addr);
        data_fd = accept(s_ftp_data_listen_sock,
                         (struct sockaddr *)&data_addr, &data_len);
      }
      if (data_fd < 0) {
        fclose(f);
        unlink(path);
        ftp_send(fd, "425 No data connection\r\n");
        continue;
      }

      ftp_send(fd, "150 Receiving file\r\n");
      char buf[1024];
      int n;
      while ((n = recv(data_fd, buf, sizeof(buf), 0)) > 0) {
        fwrite(buf, 1, (size_t)n, f);
      }
      fclose(f);
      close(data_fd);
      if (s_ftp_data_listen_sock >= 0) {
        close(s_ftp_data_listen_sock);
        s_ftp_data_listen_sock = -1;
      }
      s_ftp_pasv_mode = false;
      ftp_send(fd, "226 Transfer complete\r\n");

    } else if (strcmp(cmd, "DELE") == 0) {
      char path[256];
      if (arg[0] == '/') {
        snprintf(path, sizeof(path), "/sd%s", arg);
      } else {
        snprintf(path, sizeof(path), "/sd%s/%s", s_ftp_cwd, arg);
      }
      if (unlink(path) == 0) {
        ftp_send(fd, "250 Delete OK\r\n");
      } else {
        ftp_send(fd, "550 Delete failed\r\n");
      }

    } else if (strcmp(cmd, "MKD") == 0) {
      char path[256];
      if (arg[0] == '/') {
        snprintf(path, sizeof(path), "/sd%s", arg);
      } else {
        snprintf(path, sizeof(path), "/sd%s/%s", s_ftp_cwd, arg);
      }
      if (mkdir(path, 0755) == 0) {
        ftp_sendf(fd, "257 \"%s\" created\r\n", arg);
      } else {
        ftp_send(fd, "550 Can't create directory\r\n");
      }

    } else if (strcmp(cmd, "RMD") == 0) {
      char path[256];
      if (arg[0] == '/') {
        snprintf(path, sizeof(path), "/sd%s", arg);
      } else {
        snprintf(path, sizeof(path), "/sd%s/%s", s_ftp_cwd, arg);
      }
      if (rmdir(path) == 0) {
        ftp_send(fd, "250 Remove OK\r\n");
      } else {
        ftp_send(fd, "550 Remove failed\r\n");
      }

    } else if (strcmp(cmd, "RNFR") == 0) {
      // Simplified rename: expect RNFR + RNTO as consecutive commands
      char from_path[256];
      if (arg[0] == '/') {
        snprintf(from_path, sizeof(from_path), "/sd%s", arg);
      } else {
        snprintf(from_path, sizeof(from_path), "/sd%s/%s", s_ftp_cwd, arg);
      }

      // Read RNTO command
      memset(line, 0, sizeof(line));
      if (ftp_recv_line(fd, line, sizeof(line)) < 0) {
        ftp_send(fd, "503 Bad sequence\r\n");
        continue;
      }

      char rn_cmd[16], rn_arg[240];
      memset(rn_cmd, 0, sizeof(rn_cmd));
      memset(rn_arg, 0, sizeof(rn_arg));
      char *rn_space = strchr(line, ' ');
      if (rn_space) {
        *rn_space = '\0';
        strncpy(rn_cmd, line, sizeof(rn_cmd) - 1);
        strncpy(rn_arg, rn_space + 1, sizeof(rn_arg) - 1);
      } else {
        strncpy(rn_cmd, line, sizeof(rn_cmd) - 1);
      }
      for (char *p = rn_cmd; *p; ++p) {
        if (*p >= 'a' && *p <= 'z') { *p -= 32; }
      }

      if (strcmp(rn_cmd, "RNTO") != 0 || rn_arg[0] == '\0') {
        ftp_send(fd, "503 Bad sequence\r\n");
        continue;
      }

      char to_path[256];
      if (rn_arg[0] == '/') {
        snprintf(to_path, sizeof(to_path), "/sd%s", rn_arg);
      } else {
        snprintf(to_path, sizeof(to_path), "/sd%s/%s", s_ftp_cwd, rn_arg);
      }

      if (rename(from_path, to_path) == 0) {
        ftp_send(fd, "250 Rename OK\r\n");
      } else {
        ftp_send(fd, "550 Rename failed\r\n");
      }

    } else if (strcmp(cmd, "SIZE") == 0) {
      char path[256];
      if (arg[0] == '/') {
        snprintf(path, sizeof(path), "/sd%s", arg);
      } else {
        snprintf(path, sizeof(path), "/sd%s/%s", s_ftp_cwd, arg);
      }
      struct stat st;
      if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode)) {
        ftp_sendf(fd, "213 %lld\r\n", (long long)st.st_size);
      } else {
        ftp_send(fd, "550 Not found\r\n");
      }

    } else if (strcmp(cmd, "CWD") == 0) {
      if (arg[0] == '\0') {
        strcpy(s_ftp_cwd, "/");
      } else if (arg[0] == '/') {
        strncpy(s_ftp_cwd, arg, sizeof(s_ftp_cwd) - 1);
      } else if (strcmp(arg, "..") == 0) {
        char *slash = strrchr(s_ftp_cwd, '/');
        if (slash && slash != s_ftp_cwd) { *slash = '\0'; }
        else { strcpy(s_ftp_cwd, "/"); }
      } else {
        size_t cwd_len = strlen(s_ftp_cwd);
        if (cwd_len > 0 && s_ftp_cwd[cwd_len - 1] != '/') {
          strncat(s_ftp_cwd, "/", sizeof(s_ftp_cwd) - strlen(s_ftp_cwd) - 1);
        }
        strncat(s_ftp_cwd, arg, sizeof(s_ftp_cwd) - strlen(s_ftp_cwd) - 1);
      }
      ftp_sendf(fd, "250 CWD OK \"%s\"\r\n", s_ftp_cwd);

    } else if (strcmp(cmd, "CDUP") == 0) {
      char *slash = strrchr(s_ftp_cwd, '/');
      if (slash && slash != s_ftp_cwd) { *slash = '\0'; }
      else { strcpy(s_ftp_cwd, "/"); }
      ftp_sendf(fd, "200 CDUP OK \"%s\"\r\n", s_ftp_cwd);

    } else if (strcmp(cmd, "OPTS") == 0) {
      ftp_send(fd, "200 OK\r\n");

    } else if (strcmp(cmd, "NOOP") == 0) {
      ftp_send(fd, "200 OK\r\n");

    } else if (strcmp(cmd, "FEAT") == 0) {
      ftp_send(fd, "211-Extensions:\r\n SIZE\r\n PASV\r\n211 End\r\n");

    } else {
      ftp_sendf(fd, "502 Unknown command: %s\r\n", cmd);
    }
  }

  // Cleanup
  if (s_ftp_data_listen_sock >= 0) {
    close(s_ftp_data_listen_sock);
    s_ftp_data_listen_sock = -1;
  }
  s_ftp_pasv_mode = false;
  close(fd);
}

} // namespace

extern "C" uint8_t mia_host_ftp_start(void) {
  if (s_ftp_running) {
    return 1;
  }

  strcpy(s_ftp_status, "Starting...");

  // Start AP
  if (wifi_start_ap("MiaOS", nullptr) != ESP_OK) {
    strcpy(s_ftp_status, "WiFi fail");
    return 0;
  }
  wifi_get_ip(s_ftp_ip, sizeof(s_ftp_ip), true);

  // Create FTP control socket
  s_ftp_ctrl_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (s_ftp_ctrl_sock < 0) {
    strcpy(s_ftp_status, "Socket fail");
    wifi_ensure_stop();
    return 0;
  }

  int opt = 1;
  setsockopt(s_ftp_ctrl_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(21);

  if (bind(s_ftp_ctrl_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(s_ftp_ctrl_sock);
    s_ftp_ctrl_sock = -1;
    strcpy(s_ftp_status, "Bind fail");
    wifi_ensure_stop();
    return 0;
  }

  listen(s_ftp_ctrl_sock, 1);
  s_ftp_running = true;
  strcpy(s_ftp_status, "FTP ready");
  ESP_LOGI(FTP_TAG, "FTP server ftp://%s user=miaos pass=miaos", s_ftp_ip);
  return 1;
}

extern "C" void mia_host_ftp_poll(void) {
  if (!s_ftp_running) { return; }

  // If we already have a client, handle it
  if (s_ftp_client_sock >= 0) {
    ftp_handle_client(s_ftp_client_sock);
    s_ftp_client_sock = -1;
    return;
  }

  // Check for new connection (non-blocking)
  struct timeval tv = {0, 0};
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(s_ftp_ctrl_sock, &read_fds);
  int ret = select(s_ftp_ctrl_sock + 1, &read_fds, nullptr, nullptr, &tv);
  if (ret > 0) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    s_ftp_client_sock = accept(s_ftp_ctrl_sock,
                                (struct sockaddr *)&client_addr, &client_len);
    if (s_ftp_client_sock >= 0) {
      ESP_LOGI(FTP_TAG, "FTP client connected");
    }
  }
}

extern "C" void mia_host_ftp_stop(void) {
  s_ftp_running = false;
  if (s_ftp_client_sock >= 0) {
    close(s_ftp_client_sock);
    s_ftp_client_sock = -1;
  }
  if (s_ftp_ctrl_sock >= 0) {
    close(s_ftp_ctrl_sock);
    s_ftp_ctrl_sock = -1;
  }
  if (s_ftp_data_listen_sock >= 0) {
    close(s_ftp_data_listen_sock);
    s_ftp_data_listen_sock = -1;
  }
  wifi_ensure_stop();
  strcpy(s_ftp_status, "Stopped");
  ESP_LOGI(FTP_TAG, "stopped");
}

extern "C" uint8_t mia_host_ftp_get_status(MiaHostFtpStatus *status) {
  if (status == nullptr) {
    return 0;
  }
  memset(status, 0, sizeof(*status));
  status->running = s_ftp_running ? 1 : 0;
  strncpy(status->status, s_ftp_status, sizeof(status->status) - 1);
  strncpy(status->ssid, "MiaOS", sizeof(status->ssid) - 1);
  strncpy(status->ip, s_ftp_ip, sizeof(status->ip) - 1);
  strncpy(status->user, "miaos", sizeof(status->user) - 1);
  strncpy(status->pass, "miaos", sizeof(status->pass) - 1);
  return 1;
}
