#include "test_support.h"

#include "mia_hardware_audio.h"
#include "mia_hardware_target.h"

#include <array>

int main() {
    const MiaHardwareTarget *doom = nullptr;
    require_status(mia_hardware_target_find(&mia_hardware_generated_targets, "doom", &doom), MIA_HARDWARE_OK);
    MiaAudioConfig config{};
    require_status(mia_audio_negotiate(doom, &config), MIA_HARDWARE_OK);
    require_true(config.sample_rate_hz == 22050 && config.channels == 2 && config.bits_per_sample == 16, "doom should negotiate bounded stereo PCM at target rate");

    MiaHardwareTarget bad_rate = *doom;
    bad_rate.sample_rate_hz = 96000;
    require_status(mia_audio_negotiate(&bad_rate, &config), MIA_HARDWARE_ERR_UNSUPPORTED_RATE);

    std::array<int16_t, 8> storage{};
    MiaAudioQueue queue{};
    require_status(mia_audio_queue_init(&queue, storage.data(), 4), MIA_HARDWARE_OK);
    std::array<int16_t, 6> frames{1, 2, 3, 4, 5, 6};
    MiaAudioTransfer transfer{};
    require_status(mia_audio_queue_push(&queue, frames.data(), 3, &transfer), MIA_HARDWARE_OK);
    require_true(transfer.accepted_frames == 3 && transfer.dropped_frames == 0, "queue should accept frames within capacity");
    require_status(mia_audio_queue_push(&queue, frames.data(), 3, &transfer), MIA_HARDWARE_ERR_QUEUE_FULL);
    require_true(transfer.accepted_frames == 1 && transfer.dropped_frames == 2, "queue overflow should drop only excess frames without blocking");
    std::array<int16_t, 10> out{};
    require_status(mia_audio_queue_pop(&queue, out.data(), 5, &transfer), MIA_HARDWARE_ERR_QUEUE_EMPTY);
    require_true(transfer.accepted_frames == 4 && transfer.dropped_frames == 1, "underflow should report missing frames and leave bounded output");
    require_true(out[0] == 1 && out[1] == 2 && out[6] == 1 && out[7] == 2 && out[8] == 0 && out[9] == 0, "queue pop should preserve stereo order and zero-fill underflow");
}
