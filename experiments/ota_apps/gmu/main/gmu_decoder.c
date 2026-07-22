#include "gmudecoder.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "minimp3_ex.h"

static mp3dec_ex_t decoder;
static mp3dec_io_t io;
static FILE *file;

static size_t read_cb(void *buffer, size_t size, void *user_data) {
    FILE *stream = (FILE *)user_data;
    return stream == NULL ? 0 : fread(buffer, 1, size, stream);
}

static int seek_cb(uint64_t position, void *user_data) {
    FILE *stream = (FILE *)user_data;
    if (stream == NULL || position > LONG_MAX) return -1;
    return fseek(stream, (long)position, SEEK_SET);
}

static const char *decoder_name(void) { return "Gmu ESP32 minimp3 decoder"; }
static const char *decoder_info(void) { return "Static MP3 decoder for MiaOS"; }
static const char *decoder_extensions(void) { return ".mp3;.mp2"; }
static const char *decoder_type(void) { return "MPEG audio"; }

static int decoder_open(const char *filename) {
    memset(&decoder, 0, sizeof(decoder));
    memset(&io, 0, sizeof(io));
    file = fopen(filename, "rb");
    if (file == NULL) return 0;
    io.read = read_cb;
    io.read_data = file;
    io.seek = seek_cb;
    io.seek_data = file;
    if (mp3dec_ex_open_cb(&decoder, &io, MP3D_DO_NOT_SCAN) != 0) {
        fclose(file);
        file = NULL;
        return 0;
    }
    return 1;
}

static int decoder_close(void) {
    mp3dec_ex_close(&decoder);
    if (file != NULL) fclose(file);
    file = NULL;
    return 1;
}

static int decoder_data(char *target, size_t max_size) {
    size_t samples = max_size / sizeof(int16_t);
    size_t decoded = mp3dec_ex_read(&decoder, (int16_t *)target, samples);
    return (int)(decoded * sizeof(int16_t));
}

static int decoder_samplerate(void) { return decoder.info.hz; }
static int decoder_channels(void) { return decoder.info.channels; }
static int decoder_length(void) {
    if (decoder.info.hz == 0 || decoder.info.channels == 0) return 0;
    return (int)(decoder.samples / ((uint64_t)decoder.info.hz * decoder.info.channels));
}
static int decoder_bitrate(void) { return (int)decoder.info.bitrate_kbps * 1000; }
static int decoder_current_bitrate(void) { return decoder_bitrate(); }
static GmuCharset decoder_charset(void) { return M_CHARSET_AUTODETECT; }

GmuDecoder *gmu_register_decoder(void) {
    static GmuDecoder result = {
        .identifier = "gmu_minimp3",
        .init_decoder = NULL,
        .close_decoder = NULL,
        .get_name = decoder_name,
        .get_info = decoder_info,
        .get_file_extensions = decoder_extensions,
        .get_mime_types = NULL,
        .open_file = decoder_open,
        .close_file = decoder_close,
        .decode_data = decoder_data,
        .seek = NULL,
        .get_current_bitrate = decoder_current_bitrate,
        .get_meta_data = NULL,
        .get_meta_data_int = NULL,
        .get_samplerate = decoder_samplerate,
        .get_channels = decoder_channels,
        .get_length = decoder_length,
        .get_bitrate = decoder_bitrate,
        .get_file_type = decoder_type,
        .get_decoder_buffer_size = NULL,
        .meta_data_load = NULL,
        .meta_data_close = NULL,
        .meta_data_get_charset = decoder_charset,
        .data_check_magic_bytes = NULL,
        .set_reader_handle = NULL,
        .handle = NULL,
    };
    return &result;
}
