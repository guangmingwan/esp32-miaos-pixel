#include "mia_app_audio.h"
#include "mia_app_input.h"
#include "mia_app_save.h"
#include "mia_app_video.h"
#include "mia_hardware_display.h"
#include "mia_hardware_target.h"
#include "mia_emulator_smsplus.h"
#include "mia_storage.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

extern "C" const MiaHardwareTargetCatalog mia_hardware_generated_targets;
extern "C" const MiaStorageTargetCatalog mia_storage_generated_targets;

static void require_true(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

template <typename T>
static void require_code(T status, int expected, const char *message) {
    if (static_cast<int>(status.code) != expected) {
        std::fprintf(stderr, "FAIL: %s (%d != %d)\n", message, static_cast<int>(status.code), expected);
        std::exit(1);
    }
}

static const MiaHardwareTarget *hardware_target(const char *name) {
    const MiaHardwareTarget *target = nullptr;
    require_code(mia_hardware_target_find(&mia_hardware_generated_targets, name, &target), MIA_HARDWARE_OK, "hardware target exists");
    return target;
}

static const MiaStorageTarget *storage_target(const char *name) {
    const MiaStorageTarget *target = nullptr;
    require_code(mia_storage_target_find(&mia_storage_generated_targets, name, &target), MIA_STORAGE_OK, "storage target exists");
    return target;
}

static std::string read_file(const fs::path &path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

struct PresentedFrame {
    std::array<uint16_t, MIA_DISPLAY_PIXELS> pixels{};
    uint32_t width{};
    uint32_t height{};
    uint32_t pitch_bytes{};
    size_t calls{};
};

static int32_t capture_present(const uint16_t *pixels, uint32_t width, uint32_t height,
                               uint32_t pitch_bytes, void *context) {
    PresentedFrame &frame = *static_cast<PresentedFrame *>(context);
    std::copy_n(pixels, frame.pixels.size(), frame.pixels.begin());
    frame.width = width;
    frame.height = height;
    frame.pitch_bytes = pitch_bytes;
    ++frame.calls;
    return 0;
}

static void test_video_renders_full_geometry_exact_pixels() {
    std::array<uint16_t, MIA_DISPLAY_PIXELS> out{};
    std::vector<uint16_t> src(160u * 144u, 0x2222);
    src.front() = 0x1111;
    src.back() = 0x4444;
    MiaAppVideoSink sink{out.data(), out.size(), MIA_DISPLAY_SCALE_FIT};
    MiaDisplayRect rect{};
    require_code(mia_app_video_submit(&sink, hardware_target("gb"), src.data(), src.size(), &rect), MIA_HARDWARE_OK, "video submit succeeds");
    require_true(rect.x == 80 && rect.y == 48 && rect.width == 160 && rect.height == 144, "gb frame uses fit geometry without OOB");
    require_true(out[48 * MIA_DISPLAY_WIDTH + 80] == 0x1111, "top-left visible pixel is copied from source");
    require_true(out[191 * MIA_DISPLAY_WIDTH + 239] == 0x4444, "bottom-right visible pixel is copied from source");
}

static void test_video_presents_all_scaled_and_letterboxed_pixels() {
    std::array<uint16_t, MIA_DISPLAY_PIXELS> out{};
    std::vector<uint16_t> src(160u * 144u);
    for (size_t index = 0; index < src.size(); ++index) src[index] = static_cast<uint16_t>(index);
    PresentedFrame presented{};
    MiaAppVideoSink sink{out.data(), out.size(), MIA_DISPLAY_SCALE_FIT};

    require_code(mia_app_video_submit_to_host(&sink, hardware_target("gb"), src.data(), src.size(),
                                              capture_present, &presented),
                 MIA_HARDWARE_OK, "scaled video presents to host");

    require_true(presented.calls == 1, "host receives exactly one complete frame");
    require_true(presented.width == 320 && presented.height == 240 && presented.pitch_bytes == 640,
                 "host receives native display geometry and pitch");
    for (size_t index = 0; index < presented.pixels.size(); ++index) {
        require_true(presented.pixels[index] == out[index], "host receives every scaled pixel");
    }
    require_true(presented.pixels[0] == 0 && presented.pixels[47 * MIA_DISPLAY_WIDTH + 319] == 0,
                 "top letterbox is black");
    require_true(presented.pixels[192 * MIA_DISPLAY_WIDTH] == 0 && presented.pixels.back() == 0,
                 "bottom letterbox is black");
}

static void test_audio_is_bounded_and_reports_overflow() {
    std::array<int16_t, 8> queue_storage{};
    std::array<int16_t, 8> write_storage{};
    MiaAppAudioSink sink{};
    require_code(mia_app_audio_init(&sink, queue_storage.data(), 4), MIA_HARDWARE_OK, "audio sink initializes");
    const std::array<int16_t, 12> frames{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    MiaAudioTransfer transfer{};
    require_code(mia_app_audio_submit(&sink, frames.data(), 6, &transfer), MIA_HARDWARE_ERR_QUEUE_FULL, "audio overflow is reported");
    require_true(transfer.accepted_frames == 4 && transfer.dropped_frames == 2, "overflow reports accepted and dropped frames");
    require_code(mia_app_audio_drain(&sink, write_storage.data(), 4, &transfer), MIA_HARDWARE_OK, "audio drains accepted frames");
    require_true(write_storage[0] == 1 && write_storage[7] == 8, "audio preserves PCM frame order");
}

struct AudioCapture {
    std::vector<int16_t> samples;
};

static int32_t capture_audio(const int16_t *frames, size_t frame_count, void *context) {
    auto *capture = static_cast<AudioCapture *>(context);
    capture->samples.insert(capture->samples.end(), frames, frames + frame_count * 2u);
    return static_cast<int32_t>(frame_count);
}

static void test_audio_delivers_every_accepted_frame_in_bounded_chunks() {
    std::array<int16_t, 16> queue_storage{};
    std::array<int16_t, 4> scratch{};
    const std::array<int16_t, 12> frames{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    MiaAppAudioSink sink{};
    AudioCapture capture;
    require_code(mia_app_audio_init(&sink, queue_storage.data(), 8), MIA_HARDWARE_OK, "audio delivery sink initializes");
    require_code(mia_app_audio_deliver(&sink, frames.data(), 6, scratch.data(), 2, capture_audio, &capture), MIA_HARDWARE_OK, "audio delivery succeeds");
    require_true(capture.samples == std::vector<int16_t>(frames.begin(), frames.end()), "audio delivery preserves every accepted native sample");
    require_true(sink.queue.count_frames == 0, "audio delivery leaves no accepted frames stranded");
}

static void test_input_maps_host_buttons_and_detects_menu_press() {
    MiaAppInputState state{};
    mia_app_input_init(&state);
    const uint32_t buttons = (1u << MIA_HOST_KEY_A) | (1u << MIA_HOST_KEY_M);
    require_true((mia_app_input_core_mask(hardware_target("gb"), buttons) & MIA_APP_CORE_INPUT_A) != 0, "host A maps to core A");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_RIGHT | MIA_APP_CORE_INPUT_A | MIA_APP_CORE_INPUT_START) == 0x92u, "app buttons map to corrected gnuboy native bits");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_LEFT) == 0x01u, "host left maps to corrected gnuboy horizontal bit");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_RIGHT) == 0x02u, "host right maps to corrected gnuboy horizontal bit");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_UP) == 0x08u, "host up maps to corrected gnuboy vertical bit");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_DOWN) == 0x04u, "host down maps to corrected gnuboy vertical bit");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_LEFT | MIA_APP_CORE_INPUT_RIGHT) == 0,
                 "contradictory horizontal input is neutral");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_UP | MIA_APP_CORE_INPUT_DOWN) == 0,
                 "contradictory vertical input is neutral");
    require_true(mia_app_input_gw_mask(MIA_APP_CORE_INPUT_UP | MIA_APP_CORE_INPUT_B | MIA_APP_CORE_INPUT_SELECT) == 0x62u, "app buttons map to GW native bits");
    require_true(mia_app_input_menu_requested(&state, buttons), "M press opens the menu immediately");
    require_true(!mia_app_input_menu_requested(&state, buttons), "held M is one-shot");
    require_true(!mia_app_input_menu_requested(&state, 0), "M release rearms the menu button");
    require_true(mia_app_input_menu_requested(&state, buttons), "a new M press opens the menu again");
}

static void test_smsplus_maps_target_specific_controls() {
    const uint32_t buttons = MIA_APP_CORE_INPUT_UP | MIA_APP_CORE_INPUT_A |
                             MIA_APP_CORE_INPUT_B | MIA_APP_CORE_INPUT_START |
                             MIA_APP_CORE_INPUT_SELECT;
    MiaSmsPlusInput sms = mia_smsplus_map_input(MIA_SMSPLUS_MODE_SMS, buttons);
    MiaSmsPlusInput gg = mia_smsplus_map_input(MIA_SMSPLUS_MODE_GG, buttons);
    require_true(sms.pad == 0x31u && sms.system == 0x03u, "SMS maps buttons, pause, and start");
    require_true(gg.pad == 0x31u && gg.system == 0x03u, "Game Gear maps buttons and start");
}

static void test_smsplus_converts_complete_palette_frame_to_rgb565() {
    std::array<uint8_t, 4> indexed{0, 1, 31, 255};
    std::array<uint16_t, 256> palette{};
    palette[0] = 0x0000;
    palette[1] = 0xf800;
    palette[31] = 0x07e0;
    palette[255] = 0x001f;
    std::array<uint16_t, 4> output{};
    require_true(mia_smsplus_convert_frame(indexed.data(), indexed.size(), palette.data(),
                                           palette.size(), output.data(), output.size()),
                  "SMS Plus indexed frame converts");
    require_true(output == std::array<uint16_t, 4>{0x0000, 0xf800, 0x07e0, 0x001f},
                 "every 8-bit indexed pixel resolves through the RGB565 palette");
}

static void test_coleco_keypad_layout_matches_original() {
    const std::array<uint8_t, 12> expected{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 11};
    for (size_t index = 0; index < expected.size(); ++index) {
        require_true(mia_smsplus_coleco_keypad_code(index) == expected[index],
                     "Coleco keypad index maps to original key code");
    }
    require_true(mia_smsplus_coleco_keypad_code(expected.size()) == 0xffu,
                 "Coleco keypad rejects out-of-range index");
}

static void test_picker_requires_explicit_entry_and_atomic_save_uses_selected_rom_name() {
    const fs::path root = fs::temp_directory_path() / "mia-app-adapter-test";
    fs::remove_all(root);
    fs::create_directories(root / "roms" / "gb");
    fs::create_directories(root / "saves" / "gb");
    std::ofstream(root / "roms" / "gb" / "demo.gb", std::ios::binary).put('R');
    std::ofstream(root / "roms" / "gb" / "skip.zip", std::ios::binary).put('Z');
    MiaStorageContext context{root.c_str()};
    MiaStoragePickerResult result{};
    require_code(mia_storage_picker_list(&context, storage_target("gb"), &result), MIA_STORAGE_OK, "picker lists ROMs");
    MiaAppPickerSelection selection{};
    require_code(mia_app_picker_select_entry(&result, result.count, &selection), MIA_STORAGE_ERR_INVALID_ARGUMENT, "picker rejects missing explicit selection");
    size_t selected = 0;
    while (selected < result.count && std::string(result.entries[selected].name) != "demo.gb") ++selected;
    require_code(mia_app_picker_select_entry(&result, selected, &selection), MIA_STORAGE_OK, "picker accepts explicit ROM selection");
    require_true(std::string(selection.rom_path).ends_with("demo.gb"), "picker filters archives and selects ROM");
    require_true(std::string(selection.save_name) == "demo.sav", "save name derives from ROM stem");
    const std::array<uint8_t, 3> first{'o', 'l', 'd'};
    const std::array<uint8_t, 3> second{'n', 'e', 'w'};
    require_code(mia_app_save_flush(&context, storage_target("gb"), selection.save_name, MIA_STORAGE_FLUSH_CLEAN_EXIT, first.data(), first.size(), nullptr), MIA_STORAGE_OK, "initial save succeeds");
    MiaStorageFault fault{MIA_STORAGE_FAULT_BEFORE_REPLACE};
    require_code(mia_app_save_flush(&context, storage_target("gb"), selection.save_name, MIA_STORAGE_FLUSH_ROM_CHANGE, second.data(), second.size(), &fault), MIA_STORAGE_ERR_INTERRUPTED, "interrupted flush reports error");
    require_true(read_file(root / "saves" / "gb" / "demo.sav") == "old", "old save survives failed replace");
    size_t loaded_size = 0;
    std::array<uint8_t, 3> loaded{};
    require_code(mia_app_save_load(&context, storage_target("gb"), selection.save_name, loaded.data(), loaded.size(), &loaded_size), MIA_STORAGE_OK, "native save loads through selected safe path");
    require_true(loaded_size == first.size() && loaded == first, "native save bytes round trip unchanged");
    mia_app_picker_selection_free(&selection);
    mia_storage_picker_free(&result);
    fs::remove_all(root);
}

int main() {
    test_video_renders_full_geometry_exact_pixels();
    test_video_presents_all_scaled_and_letterboxed_pixels();
    test_audio_is_bounded_and_reports_overflow();
    test_audio_delivers_every_accepted_frame_in_bounded_chunks();
    test_input_maps_host_buttons_and_detects_menu_press();
    test_smsplus_maps_target_specific_controls();
    test_smsplus_converts_complete_palette_frame_to_rgb565();
    test_coleco_keypad_layout_matches_original();
    test_picker_requires_explicit_entry_and_atomic_save_uses_selected_rom_name();
    return 0;
}
