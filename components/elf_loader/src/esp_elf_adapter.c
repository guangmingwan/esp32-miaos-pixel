/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdint.h>
#include <sys/errno.h>
#include "esp_idf_version.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp32s3/rom/cache.h"
#include "soc/soc.h"
#include "private/elf_platform.h"

#ifdef CONFIG_ELF_LOADER_LOAD_PSRAM
#ifdef CONFIG_IDF_TARGET_ESP32S3
#define OFFSET_TEXT_VALUE   (SOC_IROM_LOW - SOC_DROM_LOW)
#endif
#define ELF_PSRAM_CACHE_ALIGN 64U
#endif

#ifdef CONFIG_ELF_LOADER_LOAD_PSRAM
static void esp_elf_sync_section(const esp_elf_sec_t *sec)
{
    const uint32_t cache_line_size = 64;
    uint32_t start;
    uint32_t end;

    if (!sec || !sec->addr || !sec->size) {
        return;
    }

    start = (uint32_t)sec->addr & ~(cache_line_size - 1);
    end = ((uint32_t)sec->addr + sec->size + cache_line_size - 1) &
          ~(cache_line_size - 1);

    Cache_WriteBack_Addr(start, end - start);
}
#endif

/**
 * @brief Allocate block of memory.
 *
 * @param n - Memory size in byte
 * @param exec - True: memory can run executable code; False: memory can R/W data
 *
 * @return Memory pointer if success or NULL if failed.
 */
void *esp_elf_malloc(uint32_t n, bool exec)
{
    uint32_t caps;

#if CONFIG_ELF_LOADER_BUS_ADDRESS_MIRROR
#ifdef CONFIG_ELF_LOADER_LOAD_PSRAM
    caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
#else
    caps = exec ? MALLOC_CAP_EXEC : MALLOC_CAP_8BIT;
#endif
#else
#ifdef CONFIG_ELF_LOADER_LOAD_PSRAM
    caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
#else
    caps = MALLOC_CAP_8BIT;
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
    caps |= MALLOC_CAP_CACHE_ALIGNED;
#endif
#endif

#ifdef CONFIG_ELF_LOADER_LOAD_PSRAM
    uint8_t *raw = (uint8_t *)heap_caps_malloc(n + ELF_PSRAM_CACHE_ALIGN + sizeof(void *), caps);
    if (!raw) {
        return NULL;
    }

    uintptr_t aligned = ((uintptr_t)raw + sizeof(void *) + ELF_PSRAM_CACHE_ALIGN - 1U) &
                        ~(uintptr_t)(ELF_PSRAM_CACHE_ALIGN - 1U);
    ((void **)aligned)[-1] = raw;
    return (void *)aligned;
#else
    return heap_caps_malloc(n, caps);
#endif
}

/**
 * @brief Free block of memory.
 *
 * @param ptr - memory block pointer allocated by "esp_elf_malloc"
 *
 * @return None
 */
void esp_elf_free(void *ptr)
{
#ifdef CONFIG_ELF_LOADER_LOAD_PSRAM
    if (!ptr) {
        return;
    }

    heap_caps_free(((void **)ptr)[-1]);
#else
    heap_caps_free(ptr);
#endif
}

/**
 * @brief Remap symbol from ".data" to ".text" section.
 *
 * @param elf  - ELF object pointer
 * @param sym  - ELF symbol table
 *
 * @return Remapped symbol value
 */
#ifdef CONFIG_ELF_LOADER_CACHE_OFFSET
uintptr_t elf_remap_text(esp_elf_t *elf, uintptr_t sym)
{
    uintptr_t mapped_sym;
    esp_elf_sec_t *sec = &elf->sec[ELF_SEC_TEXT];

    if ((sym >= sec->addr) &&
            (sym < (sec->addr + sec->size))) {
#ifdef CONFIG_ELF_LOADER_SET_MMU
        mapped_sym = sym + elf->text_off;
#else
        mapped_sym = sym + OFFSET_TEXT_VALUE;
#endif
    } else {
        mapped_sym = sym;
    }

    return mapped_sym;
}
#endif

/**
 * @brief Flush data from cache to external RAM.
 *
 * @param None
 *
 * @return None
 */
#ifdef CONFIG_ELF_LOADER_LOAD_PSRAM
void esp_elf_arch_flush(esp_elf_t *elf)
{
    if (!elf) {
        return;
    }

    for (int i = 0; i < ELF_SECS; ++i) {
        esp_elf_sync_section(&elf->sec[i]);
    }
}
#endif
