#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mia_app_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIA_SMSPLUS_COLECO_BIOS_SIZE 8192u
#define MIA_SMSPLUS_SHA1_SIZE 20u

typedef enum {
    MIA_SMSPLUS_MODE_SMS = 0,
    MIA_SMSPLUS_MODE_GG,
    MIA_SMSPLUS_MODE_COLECO,
} MiaSmsPlusMode;

typedef enum {
    MIA_SMSPLUS_BIOS_OK = 0,
    MIA_SMSPLUS_BIOS_MISSING,
    MIA_SMSPLUS_BIOS_SIZE_INVALID,
    MIA_SMSPLUS_BIOS_CHECKSUM_INVALID,
    MIA_SMSPLUS_BIOS_IO_FAILED,
    MIA_SMSPLUS_BIOS_HASH_FAILED,
} MiaSmsPlusBiosStatus;

typedef struct {
    uint8_t pad;
    uint8_t system;
} MiaSmsPlusInput;

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t size;
} MiaSmsPlusBiosReadRequest;

typedef struct {
    const uint8_t *buffer;
    size_t size;
    uint8_t digest[MIA_SMSPLUS_SHA1_SIZE];
} MiaSmsPlusBiosHashRequest;

typedef MiaSmsPlusBiosStatus (*MiaSmsPlusBiosReadFn)(void *context,
                                                     MiaSmsPlusBiosReadRequest *request);
typedef int (*MiaSmsPlusBiosHashFn)(void *context, MiaSmsPlusBiosHashRequest *request);
typedef void (*MiaSmsPlusBiosAcceptFn)(void *context, uint8_t *bios);

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    MiaSmsPlusBiosReadFn read;
    MiaSmsPlusBiosHashFn hash;
    MiaSmsPlusBiosAcceptFn accept;
    void *context;
} MiaSmsPlusBiosPipeline;

MiaSmsPlusInput mia_smsplus_map_input(MiaSmsPlusMode mode, uint32_t input);
bool mia_smsplus_convert_frame(const uint8_t *indices, size_t pixel_count,
                               const uint16_t *palette, size_t palette_count,
                               uint16_t *output, size_t output_count);
MiaSmsPlusBiosStatus mia_smsplus_validate_coleco_bios_digest(const uint8_t *digest, size_t size);
MiaSmsPlusBiosStatus mia_smsplus_load_validate_coleco_bios(const MiaSmsPlusBiosPipeline *pipeline);
const char *mia_smsplus_bios_error(MiaSmsPlusBiosStatus status);

#ifdef __cplusplus
}
#endif
