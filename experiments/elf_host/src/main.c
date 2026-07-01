#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

#include "mia_elf_runner.h"

static const char *TAG = "miaos_elf_host";

static esp_err_t mount_sd_card(void) {
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024,
  };

  sdmmc_card_t *card = NULL;
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  spi_bus_config_t bus_config = {
      .mosi_io_num = 23,
      .miso_io_num = 19,
      .sclk_io_num = 18,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };
  esp_err_t err = spi_bus_initialize(host.slot, &bus_config, SDSPI_DEFAULT_DMA);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
    return err;
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = 22;
  slot_config.host_id = host.slot;

  err = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(err));
    spi_bus_free(host.slot);
    return err;
  }
  sdmmc_card_print_info(stdout, card);
  return ESP_OK;
}

void app_main(void) {
  ESP_LOGI(TAG, "MiaOS ELF host experiment booted");
  esp_err_t err = mount_sd_card();
  if (err != ESP_OK) {
    return;
  }
  err = mia_run_first_sd_elf_app();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "ELF app run failed: %s", esp_err_to_name(err));
    return;
  }
  ESP_LOGI(TAG, "ELF app returned cleanly");
}
