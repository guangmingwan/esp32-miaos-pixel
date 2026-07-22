#include "gmudecoder.h"

#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#define DR_FLAC_IMPLEMENTATION
#include "../third_party/dr_flac.h"

#include "../third_party/stb_vorbis.c"

static int file_bitrate(const char *filename, int length_seconds) {
    struct stat info;
    if (length_seconds <= 0 || stat(filename, &info) != 0) return 0;
    return (int)(((long long)info.st_size * 8) / length_seconds);
}

/* Ogg Vorbis decoder. stb_vorbis returns interleaved signed 16-bit PCM. */
static stb_vorbis *vorbis;
static unsigned vorbis_rate;
static int vorbis_channels;
static unsigned vorbis_total_samples;
static int vorbis_bitrate;

static int vorbis_open(const char *filename) {
    int error = 0;
    vorbis = stb_vorbis_open_filename(filename, &error, NULL);
    if (vorbis == NULL) return 0;
    stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    vorbis_rate = info.sample_rate;
    vorbis_channels = info.channels;
    vorbis_total_samples = stb_vorbis_stream_length_in_samples(vorbis);
    if (vorbis_rate == 0 || vorbis_channels < 1 || vorbis_channels > 2) {
        stb_vorbis_close(vorbis);
        vorbis = NULL;
        return 0;
    }
    vorbis_bitrate = file_bitrate(filename, (int)(vorbis_total_samples / vorbis_rate));
    return 1;
}

static int vorbis_close(void) {
    if (vorbis != NULL) stb_vorbis_close(vorbis);
    vorbis = NULL;
    return 1;
}

static int vorbis_decode(char *target, size_t max_size) {
    if (vorbis == NULL || vorbis_channels <= 0) return -1;
    int max_shorts = (int)(max_size / sizeof(int16_t));
    int decoded = stb_vorbis_get_samples_short_interleaved(
        vorbis, vorbis_channels, (short *)target, max_shorts);
    return decoded * (int)sizeof(int16_t) * vorbis_channels;
}

static int vorbis_seek(int second) {
    return vorbis != NULL && stb_vorbis_seek(vorbis, (unsigned)(second * vorbis_rate));
}

static const char *vorbis_name(void) { return "Gmu stb_vorbis decoder"; }
static const char *vorbis_info(void) { return "Static Ogg Vorbis decoder for MiaOS"; }
static const char *vorbis_extensions(void) { return ".ogg;.oga"; }
static int vorbis_samplerate(void) { return (int)vorbis_rate; }
static int vorbis_get_channels(void) { return vorbis_channels; }
static int vorbis_length(void) {
    return vorbis_rate == 0 ? 0 : (int)(vorbis_total_samples / vorbis_rate);
}
static int vorbis_current_bitrate(void) { return vorbis_bitrate; }
static const char *vorbis_type(void) { return "Ogg Vorbis"; }
static GmuCharset vorbis_charset(void) { return M_CHARSET_UTF_8; }

GmuDecoder *gmu_register_vorbis_decoder(void) {
    static GmuDecoder decoder = {
        .identifier = "gmu_vorbis",
        .init_decoder = NULL,
        .close_decoder = NULL,
        .get_name = vorbis_name,
        .get_info = vorbis_info,
        .get_file_extensions = vorbis_extensions,
        .get_mime_types = NULL,
        .open_file = vorbis_open,
        .close_file = vorbis_close,
        .decode_data = vorbis_decode,
        .seek = vorbis_seek,
        .get_current_bitrate = vorbis_current_bitrate,
        .get_meta_data = NULL,
        .get_meta_data_int = NULL,
        .get_samplerate = vorbis_samplerate,
        .get_channels = vorbis_get_channels,
        .get_length = vorbis_length,
        .get_bitrate = vorbis_current_bitrate,
        .get_file_type = vorbis_type,
        .get_decoder_buffer_size = NULL,
        .meta_data_load = NULL,
        .meta_data_close = NULL,
        .meta_data_get_charset = vorbis_charset,
        .data_check_magic_bytes = NULL,
        .set_reader_handle = NULL,
        .handle = NULL,
    };
    return &decoder;
}

/* Native FLAC decoder. dr_flac outputs interleaved signed 16-bit PCM. */
static drflac *flac;
static unsigned flac_rate;
static unsigned flac_channels;
static drflac_uint64 flac_total_frames;
static int flac_bitrate;

static int flac_open(const char *filename) {
    flac = drflac_open_file(filename, NULL);
    if (flac == NULL || flac->channels < 1 || flac->channels > 2 || flac->sampleRate == 0) {
        if (flac != NULL) drflac_close(flac);
        flac = NULL;
        return 0;
    }
    flac_rate = flac->sampleRate;
    flac_channels = flac->channels;
    flac_total_frames = flac->totalPCMFrameCount;
    flac_bitrate = file_bitrate(filename, (int)(flac_total_frames / flac_rate));
    return 1;
}

static int flac_close(void) {
    if (flac != NULL) drflac_close(flac);
    flac = NULL;
    return 1;
}

static int flac_decode(char *target, size_t max_size) {
    if (flac == NULL || flac_channels == 0) return -1;
    drflac_uint64 frames = max_size / (sizeof(int16_t) * flac_channels);
    drflac_uint64 decoded = drflac_read_pcm_frames_s16(flac, frames, (drflac_int16 *)target);
    return (int)(decoded * flac_channels * sizeof(int16_t));
}

static int flac_seek(int second) {
    return flac != NULL && drflac_seek_to_pcm_frame(flac, (drflac_uint64)second * flac_rate);
}

static const char *flac_name(void) { return "Gmu dr_flac decoder"; }
static const char *flac_info(void) { return "Static FLAC decoder for MiaOS"; }
static const char *flac_extensions(void) { return ".flac"; }
static int flac_samplerate(void) { return (int)flac_rate; }
static int flac_get_channels(void) { return (int)flac_channels; }
static int flac_length(void) {
    return flac_rate == 0 ? 0 : (int)(flac_total_frames / flac_rate);
}
static int flac_current_bitrate(void) { return flac_bitrate; }
static const char *flac_type(void) { return "FLAC"; }
static GmuCharset flac_charset(void) { return M_CHARSET_UTF_8; }

GmuDecoder *gmu_register_flac_decoder(void) {
    static GmuDecoder decoder = {
        .identifier = "gmu_flac",
        .init_decoder = NULL,
        .close_decoder = NULL,
        .get_name = flac_name,
        .get_info = flac_info,
        .get_file_extensions = flac_extensions,
        .get_mime_types = NULL,
        .open_file = flac_open,
        .close_file = flac_close,
        .decode_data = flac_decode,
        .seek = flac_seek,
        .get_current_bitrate = flac_current_bitrate,
        .get_meta_data = NULL,
        .get_meta_data_int = NULL,
        .get_samplerate = flac_samplerate,
        .get_channels = flac_get_channels,
        .get_length = flac_length,
        .get_bitrate = flac_current_bitrate,
        .get_file_type = flac_type,
        .get_decoder_buffer_size = NULL,
        .meta_data_load = NULL,
        .meta_data_close = NULL,
        .meta_data_get_charset = flac_charset,
        .data_check_magic_bytes = NULL,
        .set_reader_handle = NULL,
        .handle = NULL,
    };
    return &decoder;
}
