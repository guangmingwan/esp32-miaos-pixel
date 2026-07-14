#include "mia_app_zip.h"

#include <rom/miniz.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define ZIP_LOCAL_MAGIC 0x04034b50u
#define ZIP_METHOD_STORED 0u
#define ZIP_METHOD_DEFLATE 8u
#define ZIP_SEARCH_LIMIT 0x10000u

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint16_t compression;
    uint16_t modified_time;
    uint16_t modified_date;
    uint32_t checksum;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_size;
    uint16_t extra_field_size;
} MiaZipLocalHeader;

static bool extension_matches(const char *name, const char *const *extensions,
                              size_t extension_count) {
    const char *dot = strrchr(name, '.');
    if (dot == NULL || dot[1] == '\0') return false;
    for (size_t i = 0; i < extension_count; ++i) {
        if (extensions[i] != NULL && strcasecmp(dot + 1, extensions[i]) == 0) return true;
    }
    return false;
}

static bool find_entry(FILE *file, const char *const *extensions, size_t extension_count,
                       MiaZipLocalHeader *header, size_t *data_offset,
                       char *out_name, size_t out_name_size) {
    for (size_t offset = 0; offset < ZIP_SEARCH_LIMIT;) {
        if (fseek(file, (long)offset, SEEK_SET) != 0 || fread(header, sizeof(*header), 1, file) != 1 ||
            header->magic != ZIP_LOCAL_MAGIC || header->filename_size == 0u) return false;
        char name[256];
        if (header->filename_size >= sizeof(name) ||
            fread(name, 1, header->filename_size, file) != header->filename_size) return false;
        name[header->filename_size] = '\0';
        const size_t payload = offset + sizeof(*header) + header->filename_size + header->extra_field_size;
        if (extension_matches(name, extensions, extension_count)) {
            *data_offset = payload;
            if (out_name != NULL && out_name_size > 0u) snprintf(out_name, out_name_size, "%s", name);
            return true;
        }
        offset = payload + header->compressed_size;
    }
    return false;
}

static bool extract_entry(FILE *file, const MiaZipLocalHeader *header, size_t data_offset,
                          uint8_t *output) {
    if (fseek(file, (long)data_offset, SEEK_SET) != 0) return false;
    bool success = false;
    if (header->compression == ZIP_METHOD_STORED) {
        success = header->compressed_size == header->uncompressed_size &&
                  fread(output, 1, header->uncompressed_size, file) == header->uncompressed_size;
    } else if (header->compressed_size > 0u) {
        const size_t input_capacity = header->compressed_size < 0x8000u ? header->compressed_size : 0x8000u;
        uint8_t *input = malloc(input_capacity);
        tinfl_decompressor *decompressor = malloc(sizeof(*decompressor));
        if (input != NULL && decompressor != NULL) {
            size_t stream_remaining = header->compressed_size;
            size_t output_pos = 0;
            tinfl_status status;
            tinfl_init(decompressor);
            do {
                size_t input_size = stream_remaining < input_capacity ? stream_remaining : input_capacity;
                size_t output_size = header->uncompressed_size - output_pos;
                if (input_size == 0u || fread(input, 1, input_size, file) != input_size) {
                    status = TINFL_STATUS_FAILED;
                    break;
                }
                stream_remaining -= input_size;
                status = tinfl_decompress(decompressor, input, &input_size, output,
                                          output + output_pos, &output_size,
                                          TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF |
                                              (stream_remaining > 0u ? TINFL_FLAG_HAS_MORE_INPUT : 0u));
                output_pos += output_size;
            } while (status == TINFL_STATUS_NEEDS_MORE_INPUT);
            success = status == TINFL_STATUS_DONE && output_pos == header->uncompressed_size;
        }
        free(decompressor);
        free(input);
    }
    return success;
}

static MiaCoreStatus open_entry(const char *path, const char *const *extensions,
                                size_t extension_count, size_t capacity, FILE **out_file,
                                MiaZipLocalHeader *header, size_t *data_offset,
                                char *out_name, size_t out_name_size) {
    if (path == NULL || extensions == NULL || extension_count == 0u || capacity == 0u ||
        out_file == NULL || header == NULL || data_offset == NULL)
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid ZIP extraction request");
    FILE *file = fopen(path, "rb");
    if (file == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "ZIP open failed");
    if (!find_entry(file, extensions, extension_count, header, data_offset,
                    out_name, out_name_size) ||
        (header->flags & 0x0001u) != 0u || (header->flags & 0x0008u) != 0u ||
        (header->compression != ZIP_METHOD_STORED && header->compression != ZIP_METHOD_DEFLATE) ||
        header->uncompressed_size == 0u || header->uncompressed_size > capacity) {
        fclose(file);
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "ZIP entry corrupt or unsupported");
    }
    *out_file = file;
    return mia_core_ok();
}

MiaCoreStatus mia_app_zip_extract(const char *path,
                                  const char *const *extensions, size_t extension_count,
                                  size_t max_size, uint8_t **out_data, size_t *out_size,
                                  char *out_name, size_t out_name_size) {
    if (out_data == NULL || out_size == NULL)
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid ZIP output");
    *out_data = NULL;
    *out_size = 0u;
    FILE *file = NULL;
    MiaZipLocalHeader header = {0};
    size_t data_offset = 0;
    MiaCoreStatus status = open_entry(path, extensions, extension_count, max_size, &file,
                                      &header, &data_offset, out_name, out_name_size);
    if (status.code != MIA_CORE_OK) return status;
    uint8_t *output = malloc(header.uncompressed_size);
    if (output == NULL) {
        fclose(file);
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "ZIP output allocation failed");
    }
    const bool success = extract_entry(file, &header, data_offset, output);

    fclose(file);
    if (!success) {
        free(output);
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "ZIP decompression failed");
    }
    *out_data = output;
    *out_size = header.uncompressed_size;
    return mia_core_ok();
}

MiaCoreStatus mia_app_zip_extract_into(const char *path,
                                       const char *const *extensions, size_t extension_count,
                                       uint8_t *output, size_t output_capacity, size_t *out_size,
                                       char *out_name, size_t out_name_size) {
    if (output == NULL || out_size == NULL)
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid ZIP output");
    *out_size = 0u;
    FILE *file = NULL;
    MiaZipLocalHeader header = {0};
    size_t data_offset = 0;
    MiaCoreStatus status = open_entry(path, extensions, extension_count, output_capacity,
                                      &file, &header, &data_offset, out_name, out_name_size);
    if (status.code != MIA_CORE_OK) return status;
    const bool success = extract_entry(file, &header, data_offset, output);
    fclose(file);
    if (!success) return mia_core_error(MIA_CORE_ERR_CALLBACK, "ZIP decompression failed");
    *out_size = header.uncompressed_size;
    return mia_core_ok();
}
