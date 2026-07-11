#include "test_support.h"

#include "mia_hardware_audio.h"
#include "mia_hardware_display.h"
#include "mia_hardware_input.h"

#include <array>

int main() {
    MiaDisplayRect rect{};
    require_status(mia_display_fit_rect({0, 10}, MIA_DISPLAY_SCALE_FIT, &rect), MIA_HARDWARE_ERR_INVALID_ARGUMENT);
    std::array<uint8_t, 1> indices{2};
    std::array<uint16_t, 2> palette{0, 1};
    auto out = pixels(MIA_DISPLAY_PIXELS - 1, 0);
    MiaPalettedSurface paletted{1, 1, 1, indices.data(), palette.data(), palette.size()};
    require_status(mia_display_render_paletted(&paletted, MIA_DISPLAY_SCALE_FIT, out.data(), out.size(), &rect), MIA_HARDWARE_ERR_BUFFER_TOO_SMALL);
    out = pixels(MIA_DISPLAY_PIXELS, 0);
    require_status(mia_display_render_paletted(&paletted, MIA_DISPLAY_SCALE_FIT, out.data(), out.size(), &rect), MIA_HARDWARE_ERR_PALETTE_INDEX);

    MiaAudioQueue queue{};
    require_status(mia_audio_queue_init(&queue, nullptr, 8), MIA_HARDWARE_ERR_INVALID_ARGUMENT);
    std::array<int16_t, 4> storage{};
    require_status(mia_audio_queue_init(&queue, storage.data(), 2), MIA_HARDWARE_OK);
    MiaAudioTransfer transfer{};
    require_status(mia_audio_queue_push(&queue, nullptr, 1, &transfer), MIA_HARDWARE_ERR_INVALID_ARGUMENT);
    require_status(mia_input_map_host(nullptr, MIA_HOST_KEY_A, nullptr), MIA_HARDWARE_ERR_INVALID_ARGUMENT);
}
