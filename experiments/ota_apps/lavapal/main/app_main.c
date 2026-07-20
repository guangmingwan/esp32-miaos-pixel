#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>
#include <soc/soc.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if __has_include(<esp_dlfcn.h>)
#include <esp_dlfcn.h>
#include <esp_elf.h>
#define MIA_HAS_ESP_DLOPEN 1
#endif

#include "display_host.h"
#include "lavapal_paths.h"
#include "mia_host_abi.h"
#include "mia_sdl_api.h"

extern int lavapal_main_impl(int argc, char *argv[]);
extern esp_err_t host_platform_init(void);
extern void mia_sdl_set_api(const MiaSdlApi *api);
extern uint64_t __udivdi3(uint64_t dividend, uint64_t divisor);

#ifdef MIA_HAS_ESP_DLOPEN
static const struct esp_elfsym s_sdl_host_symbols[] = {
    ESP_ELFSYM_EXPORT(clock_gettime),
    ESP_ELFSYM_EXPORT(usleep),
    ESP_ELFSYM_EXPORT(memcpy),
    ESP_ELFSYM_EXPORT(malloc),
    ESP_ELFSYM_EXPORT(calloc),
    ESP_ELFSYM_EXPORT(free),
    ESP_ELFSYM_EXPORT(vsnprintf),
    ESP_ELFSYM_EXPORT(__udivdi3),
    ESP_ELFSYM_EXPORT(fopen),
    ESP_ELFSYM_EXPORT(fclose),
    ESP_ELFSYM_EXPORT(fread),
    ESP_ELFSYM_EXPORT(fwrite),
    ESP_ELFSYM_EXPORT(fseek),
    ESP_ELFSYM_EXPORT(access),
    ESP_ELFSYM_EXPORT(scandir),
    ESP_ELFSYM_EXPORT(alphasort),
    ESP_ELFSYM_EXPORT(dlsym),
    ESP_ELFSYM_EXPORT(mia_host_audio_open),
    ESP_ELFSYM_EXPORT(mia_host_audio_stop),
    ESP_ELFSYM_EXPORT(mia_host_audio_close),
    ESP_ELFSYM_EXPORT(mia_host_buttons_poll),
    ESP_ELFSYM_EXPORT(mia_host_button_down),
    ESP_ELFSYM_EXPORT(mia_host_present_rgb565),
    ESP_ELFSYM_END,
};
#endif

typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
} OtaEntry;

static void set_entry(OtaEntry *entry, uint32_t sequence, uint32_t state) {
    memset(entry, 0, sizeof(*entry));
    entry->ota_seq = sequence;
    entry->ota_state = state;
    entry->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&entry->ota_seq, 4);
}

static void switch_to_launcher(void) {
    const esp_partition_t *otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (otadata == NULL) return;

    OtaEntry entries[2];
    set_entry(&entries[0], 1, ESP_OTA_IMG_VALID);
    set_entry(&entries[1], 3, ESP_OTA_IMG_VALID);
    if (esp_partition_erase_range(otadata, 0, otadata->size) != ESP_OK) return;
    if (esp_partition_write(otadata, 0, &entries[0], sizeof(entries[0])) != ESP_OK) return;
    (void)esp_partition_write(otadata, 4096, &entries[1], sizeof(entries[1]));
}

static int select_game_directory(void) {
    static const char *const candidates[] = {
        LAVAPAL_APP_DIRECTORY,
        "/sd/Games/lava_pal.app",
        "/sd/lava_pal",
    };
    char path[320];

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        FILE *probe;
        snprintf(path, sizeof(path), "%s/PAT.MKF", candidates[i]);
        probe = fopen(path, "rb");
        if (probe == NULL) {
            ESP_LOGW("lava_pal", "missing %s: errno=%d", path, errno);
            continue;
        }
        fclose(probe);
        snprintf(path, sizeof(path), "%s/DATA.MKF", candidates[i]);
        probe = fopen(path, "rb");
        if (probe == NULL) {
            ESP_LOGW("lava_pal", "missing %s: errno=%d", path, errno);
            continue;
        }
        fclose(probe);

        //
        // chdir() is broken on this ESP32-S3 VFS layer (returns errno=88).
        // Instead, write an absolute-path config so PAL_LoadConfig can find
        // the game data without relying on the current working directory.
        //
        const char *const config_path = LAVAPAL_CONFIG_PATH;
        FILE *cfg = fopen(config_path, "w");
        if (cfg != NULL) {
            fprintf(cfg, "GamePath=%s\n", candidates[i]);
            fprintf(cfg, "SavePath=%s\n", candidates[i]);
            fprintf(cfg, "WindowWidth=320\n");
            fprintf(cfg, "WindowHeight=240\n");
            fprintf(cfg, "TextureWidth=320\n");
            fprintf(cfg, "TextureHeight=200\n");
            fclose(cfg);
            ESP_LOGI("lava_pal", "wrote %s with GamePath=%s", config_path, candidates[i]);
        } else {
            ESP_LOGW("lava_pal", "failed to write %s: errno=%d", config_path, errno);
        }

        ESP_LOGI("lava_pal", "using game data at %s", candidates[i]);
        return 1;
    }
    return 0;
}

static int load_sdl_library(void) {
#ifdef MIA_HAS_ESP_DLOPEN
    int register_result = esp_elf_register_symbol(s_sdl_host_symbols);
    if (register_result != 0 && register_result != -EEXIST) {
        ESP_LOGE("lava_pal", "register SDL host symbols failed: %d", register_result);
        return 0;
    }

    void *handle = dlopen(MIA_SDL_LIBRARY_NAME, RTLD_NOW);
    if (handle == NULL) {
        const char *err = dlerror();
        ESP_LOGE("lava_pal", "dlopen %s failed: %s", MIA_SDL_LIBRARY_PATH,
                 err ? err : "unknown");
        return 0;
    }
    MiaSdlGetApiFn get_api = (MiaSdlGetApiFn)dlsym(handle, "mia_sdl_get_api");
    if (get_api == NULL) {
        ESP_LOGE("lava_pal", "mia_sdl_get_api symbol missing");
        return 0;
    }
#if defined(CONFIG_IDF_TARGET_ESP32S3) && defined(CONFIG_ELF_LOADER_LOAD_PSRAM)
    uintptr_t get_api_addr = (uintptr_t)get_api;
    if (get_api_addr >= SOC_DROM_LOW && get_api_addr < SOC_DROM_HIGH) {
        get_api = (MiaSdlGetApiFn)(get_api_addr + (SOC_IROM_LOW - SOC_DROM_LOW));
    }
#endif
    const MiaSdlApi *api = get_api(MIA_SDL_ABI_VERSION);
    if (api == NULL) {
        ESP_LOGE("lava_pal", "SDL ABI v%u not supported by library", MIA_SDL_ABI_VERSION);
        return 0;
    }
    mia_sdl_set_api(api);
    ESP_LOGI("lava_pal", "loaded %s (abi=%u, size=%u)", MIA_SDL_LIBRARY_PATH,
             api->abi_version, api->struct_size);
    return 1;
#else
    ESP_LOGE("lava_pal", "elf_loader not available, cannot load SDL library");
    return 0;
#endif
}

static void game_task(void *argument) {
    char *argv[] = {(char *)"lava_pal"};
    (void)argument;

    if (!load_sdl_library()) {
        ESP_LOGE("lava_pal", "SDL library not available, cannot start game");
    } else if (!select_game_directory()) {
        ESP_LOGE("lava_pal", "game data not found on SD card");
    } else {
        (void)lavapal_main_impl(1, argv);
    }

    switch_to_launcher();
    esp_restart();
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_ERROR_CHECK(host_platform_init());
    display_host_backlight_set(0);
    display_host_fill_screen_rgb565(0);
    display_host_backlight_set(1);
    BaseType_t task_result =
        xTaskCreatePinnedToCore(game_task, "lava_pal", 96 * 1024, NULL, 5, NULL, 1);
    if (task_result != pdPASS) {
        ESP_LOGE("lava_pal", "failed to create game task: %ld", (long)task_result);
        switch_to_launcher();
        esp_restart();
    }
}
