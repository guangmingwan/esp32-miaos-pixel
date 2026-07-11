#include "mia_emulator_smsplus.h"

#include <string.h>

MiaSmsPlusInput mia_smsplus_map_input(MiaSmsPlusMode mode, uint32_t input) {
    MiaSmsPlusInput mapped = {0};
    if (input & MIA_APP_CORE_INPUT_UP) mapped.pad |= 0x01u;
    if (input & MIA_APP_CORE_INPUT_DOWN) mapped.pad |= 0x02u;
    if (input & MIA_APP_CORE_INPUT_LEFT) mapped.pad |= 0x04u;
    if (input & MIA_APP_CORE_INPUT_RIGHT) mapped.pad |= 0x08u;
    if (input & MIA_APP_CORE_INPUT_B) mapped.pad |= 0x10u;
    if (input & MIA_APP_CORE_INPUT_A) mapped.pad |= 0x20u;
    switch (mode) {
    case MIA_SMSPLUS_MODE_SMS:
        if (input & MIA_APP_CORE_INPUT_START) mapped.system |= 0x02u;
        if (input & MIA_APP_CORE_INPUT_SELECT) mapped.system |= 0x01u;
        break;
    case MIA_SMSPLUS_MODE_GG:
        if (input & MIA_APP_CORE_INPUT_START) mapped.system |= 0x01u;
        if (input & MIA_APP_CORE_INPUT_SELECT) mapped.system |= 0x02u;
        break;
    case MIA_SMSPLUS_MODE_COLECO:
        break;
    }
    return mapped;
}

bool mia_smsplus_convert_frame(const uint8_t *indices, size_t pixel_count,
                               const uint16_t *palette, size_t palette_count,
                               uint16_t *output, size_t output_count) {
    if (indices == NULL || palette == NULL || output == NULL || output_count < pixel_count) return false;
    for (size_t index = 0; index < pixel_count; ++index) {
        if (indices[index] >= palette_count) return false;
        output[index] = palette[indices[index]];
    }
    return true;
}

MiaSmsPlusBiosStatus mia_smsplus_validate_coleco_bios_digest(const uint8_t *digest, size_t size) {
    static const uint8_t canonical_ntsc[MIA_SMSPLUS_SHA1_SIZE] = {
        0x45, 0xbe, 0xdc, 0x4c, 0xbd, 0xea, 0xc6, 0x6c, 0x7d, 0xf5,
        0x9e, 0x9e, 0x59, 0x91, 0x95, 0xc7, 0x78, 0xd8, 0x6a, 0x92,
    };
    if (digest == NULL) return MIA_SMSPLUS_BIOS_MISSING;
    if (size != MIA_SMSPLUS_COLECO_BIOS_SIZE) return MIA_SMSPLUS_BIOS_SIZE_INVALID;
    if (memcmp(digest, canonical_ntsc, sizeof(canonical_ntsc)) != 0) return MIA_SMSPLUS_BIOS_CHECKSUM_INVALID;
    return MIA_SMSPLUS_BIOS_OK;
}

MiaSmsPlusBiosStatus mia_smsplus_load_validate_coleco_bios(const MiaSmsPlusBiosPipeline *pipeline) {
    if (pipeline == NULL || pipeline->buffer == NULL || pipeline->read == NULL ||
        pipeline->hash == NULL || pipeline->accept == NULL ||
        pipeline->capacity < MIA_SMSPLUS_COLECO_BIOS_SIZE + 1u) {
        return MIA_SMSPLUS_BIOS_IO_FAILED;
    }
    MiaSmsPlusBiosReadRequest read_request = {pipeline->buffer, pipeline->capacity, 0};
    MiaSmsPlusBiosStatus status = pipeline->read(pipeline->context, &read_request);
    if (status != MIA_SMSPLUS_BIOS_OK) return status;
    if (read_request.size != MIA_SMSPLUS_COLECO_BIOS_SIZE) return MIA_SMSPLUS_BIOS_SIZE_INVALID;
    MiaSmsPlusBiosHashRequest hash_request = {pipeline->buffer, read_request.size, {0}};
    if (pipeline->hash(pipeline->context, &hash_request) != 0) {
        return MIA_SMSPLUS_BIOS_HASH_FAILED;
    }
    status = mia_smsplus_validate_coleco_bios_digest(hash_request.digest, read_request.size);
    if (status != MIA_SMSPLUS_BIOS_OK) return status;
    pipeline->accept(pipeline->context, pipeline->buffer);
    return MIA_SMSPLUS_BIOS_OK;
}

const char *mia_smsplus_bios_error(MiaSmsPlusBiosStatus status) {
    switch (status) {
    case MIA_SMSPLUS_BIOS_OK:
        return "";
    case MIA_SMSPLUS_BIOS_MISSING:
        return "Coleco BIOS missing: add /bios/coleco.rom (8192 bytes)";
    case MIA_SMSPLUS_BIOS_SIZE_INVALID:
        return "Coleco BIOS invalid size: /bios/coleco.rom must be 8192 bytes";
    case MIA_SMSPLUS_BIOS_CHECKSUM_INVALID:
        return "Coleco BIOS checksum mismatch: use canonical NTSC /bios/coleco.rom";
    case MIA_SMSPLUS_BIOS_IO_FAILED:
        return "Coleco BIOS read failed: check /bios/coleco.rom and SD card";
    case MIA_SMSPLUS_BIOS_HASH_FAILED:
        return "Coleco BIOS SHA1 failed: relaunch or check firmware";
    }
    return "Coleco BIOS validation failed";
}
