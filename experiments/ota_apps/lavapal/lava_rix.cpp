#include "../common_host/lava_rix.h"

#include <stdarg.h>
#include <stdio.h>

#include "emuopl.h"
#include "player.h"
#include "rix.h"

extern "C" void AdPlug_LogWrite(const char *, ...) {}
extern "C" void AdPlug_LogFile(const char *) {}

CPlayer::CPlayer(Copl *newopl) : opl(newopl), db(nullptr) {}
CPlayer::~CPlayer() {}
const unsigned short CPlayer::note_table[12] = {};
const unsigned char CPlayer::op_table[9] = {};

struct LavaRixPlayer {
    CEmuopl opl;
    CrixPlayer *player;
    int song;
    uint32_t sample_rate;
    int16_t *frame_buffer;
    uint32_t buffered_frames;
    uint32_t buffered_offset;
};

static constexpr uint32_t RIX_TICK_FRAMES = 315;

LavaRixPlayer *lava_rix_open(const char *path, int song, uint32_t sample_rate) {
    if (path == nullptr || sample_rate == 0) return nullptr;
    FILE *probe = fopen(path, "rb");
    if (probe == nullptr) {
        printf("[LAVA][RIX] fopen failed path=%s\n", path);
        return nullptr;
    }
    fseek(probe, 0, SEEK_END);
    const long file_size = ftell(probe);
    fseek(probe, 0, SEEK_SET);
    unsigned char header[4] = {};
    fread(header, 1, sizeof(header), probe);
    fclose(probe);
    printf("[LAVA][RIX] probe size=%ld head=%02x%02x%02x%02x\n", file_size,
           header[0], header[1], header[2], header[3]);
    LavaRixPlayer *result = new LavaRixPlayer{
        CEmuopl((int)sample_rate, true, true),
        nullptr,
        song,
        sample_rate,
        nullptr,
        0,
        0,
    };
    result->player = new CrixPlayer(&result->opl);
    result->frame_buffer = new int16_t[RIX_TICK_FRAMES * 2];
    if (result->frame_buffer == nullptr ||
        result->player == nullptr ||
        !result->player->load(std::string(path), CProvider_Filesystem())) {
        printf("[LAVA][RIX] load failed song=%d\n", song);
        delete result->player;
        delete[] result->frame_buffer;
        delete result;
        return nullptr;
    }
    result->player->rewind(song);
    return result;
}

void lava_rix_close(LavaRixPlayer *player) {
    if (player == nullptr) return;
    delete player->player;
    delete[] player->frame_buffer;
    delete player;
}

int lava_rix_render(LavaRixPlayer *player, int16_t *stereo_samples,
                    uint32_t frames, int loop) {
    if (player == nullptr || stereo_samples == nullptr || frames == 0) return 0;
    uint32_t rendered = 0;
    while (rendered < frames) {
        if (player->buffered_offset >= player->buffered_frames) {
            if (!player->player->update()) {
                if (!loop) {
                    for (uint32_t i = rendered * 2; i < frames * 2; ++i)
                        stereo_samples[i] = 0;
                    return (int)frames;
                }
                player->player->rewind(player->song);
                if (!player->player->update()) return (int)rendered;
            }
            player->opl.update(player->frame_buffer, (int)RIX_TICK_FRAMES);
            player->buffered_frames = RIX_TICK_FRAMES;
            player->buffered_offset = 0;
        }
        uint32_t available = player->buffered_frames - player->buffered_offset;
        uint32_t needed = frames - rendered;
        uint32_t copy_frames = available < needed ? available : needed;
        const int16_t *source = player->frame_buffer + player->buffered_offset * 2;
        for (uint32_t i = 0; i < copy_frames * 2; ++i)
            stereo_samples[rendered * 2 + i] = source[i];
        player->buffered_offset += copy_frames;
        rendered += copy_frames;
    }
    return (int)rendered;
}
