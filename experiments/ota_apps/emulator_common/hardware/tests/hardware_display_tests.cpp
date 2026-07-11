#include "test_support.h"

#include "mia_hardware_display.h"
#include "mia_hardware_target.h"

#include <array>
#include <cstddef>

int main() {
    const MiaHardwareTarget *target = nullptr;
    require_status(mia_hardware_target_find(&mia_hardware_generated_targets, "gb", &target), MIA_HARDWARE_OK);
    MiaDisplayRect rect{};
    require_status(mia_display_fit_rect(target->geometry, MIA_DISPLAY_SCALE_FIT, &rect), MIA_HARDWARE_OK);
    require_true(rect.x == 80 && rect.y == 48 && rect.width == 160 && rect.height == 144 && rect.scale == 1, "gb fit should preserve aspect with centered integer scaling");
    require_status(mia_display_fit_rect(target->geometry, MIA_DISPLAY_SCALE_STRETCH, &rect), MIA_HARDWARE_OK);
    require_true(rect.x == 0 && rect.y == 0 && rect.width == 320 && rect.height == 240,
                 "gb stretch should fill the complete display");

    require_status(mia_hardware_target_find(&mia_hardware_generated_targets, "doom", &target), MIA_HARDWARE_OK);
    require_status(mia_display_fit_rect(target->geometry, MIA_DISPLAY_SCALE_FIT, &rect), MIA_HARDWARE_OK);
    require_true(rect.x == 0 && rect.y == 20 && rect.width == 320 && rect.height == 200 && rect.scale == 1, "doom fit should letterbox vertically");
    require_status(mia_display_fit_rect(target->geometry, MIA_DISPLAY_SCALE_CROP, &rect), MIA_HARDWARE_OK);
    require_true(rect.x == 0 && rect.y == 0 && rect.width == 320 && rect.height == 240 && rect.scale == 2, "doom crop should fill screen at integer scale");

    for (std::size_t index = 0; index < mia_hardware_generated_targets.count; ++index) {
        require_status(mia_display_fit_rect(mia_hardware_generated_targets.targets[index].geometry, MIA_DISPLAY_SCALE_FIT, &rect), MIA_HARDWARE_OK);
        require_true(rect.x + rect.width <= MIA_DISPLAY_WIDTH && rect.y + rect.height <= MIA_DISPLAY_HEIGHT, "fit rect must stay inside screen for every target");
    }

    std::array<uint8_t, 4> indices{0, 1, 2, 3};
    std::array<uint16_t, 4> palette{0x0000, 0xf800, 0x07e0, 0x001f};
    auto out = pixels(MIA_DISPLAY_PIXELS, 0xffff);
    MiaPalettedSurface paletted{2, 2, 2, indices.data(), palette.data(), palette.size()};
    require_status(mia_display_render_paletted(&paletted, MIA_DISPLAY_SCALE_FIT, out.data(), out.size(), &rect), MIA_HARDWARE_OK);
    require_true(out[119 * MIA_DISPLAY_WIDTH + 159] == 0x0000, "paletted top-left source pixel should map exactly");
    require_true(out[119 * MIA_DISPLAY_WIDTH + 160] == 0xf800, "paletted top-right source pixel should map exactly");
    require_true(out[120 * MIA_DISPLAY_WIDTH + 159] == 0x07e0, "paletted bottom-left source pixel should map exactly");
    require_true(out[120 * MIA_DISPLAY_WIDTH + 160] == 0x001f, "paletted bottom-right source pixel should map exactly");

    std::array<uint16_t, 4> rgb{0x1111, 0x2222, 0x3333, 0x4444};
    MiaRgb565Surface rgb565{2, 2, 2, rgb.data()};
    require_status(mia_display_render_rgb565(&rgb565, MIA_DISPLAY_SCALE_FIT, out.data(), out.size(), &rect), MIA_HARDWARE_OK);
    require_true(out[119 * MIA_DISPLAY_WIDTH + 159] == 0x1111 && out[120 * MIA_DISPLAY_WIDTH + 160] == 0x4444, "rgb565 source pixels should copy without palette conversion");
    require_status(mia_display_render_rgb565(&rgb565, MIA_DISPLAY_SCALE_STRETCH, out.data(), out.size(), &rect), MIA_HARDWARE_OK);
    require_true(out.front() == 0x1111 && out[MIA_DISPLAY_WIDTH - 1] == 0x2222 &&
                     out[(MIA_DISPLAY_HEIGHT - 1) * MIA_DISPLAY_WIDTH] == 0x3333 && out.back() == 0x4444,
                 "rgb565 stretch should map source corners to display corners");

    MiaDisplayBufferPolicy policy{MIA_DISPLAY_BUFFER_DOUBLE, MIA_DISPLAY_MEMORY_PSRAM};
    MiaDisplayBufferDecision decision{};
    require_status(mia_display_plan_buffers({320, 240}, policy, &decision), MIA_HARDWARE_OK);
    require_true(decision.buffer_count == 2 && decision.bytes_per_buffer == MIA_DISPLAY_PIXELS * 2 && decision.memory == MIA_DISPLAY_MEMORY_PSRAM, "double-buffer PSRAM policy should be explicit");
}
