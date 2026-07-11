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

static void test_input_maps_host_buttons_and_debounces_exit() {
    MiaAppInputState state{};
    mia_app_input_init(&state, 250);
    const uint32_t buttons = (1u << MIA_HOST_KEY_A) | (1u << MIA_HOST_KEY_SELECT) | (1u << MIA_HOST_KEY_START);
    require_true((mia_app_input_core_mask(hardware_target("gb"), buttons) & MIA_APP_CORE_INPUT_A) != 0, "host A maps to core A");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_RIGHT | MIA_APP_CORE_INPUT_A | MIA_APP_CORE_INPUT_START) == 0x91u, "app buttons map to gnuboy native bits");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_LEFT) == 0x02u, "host left maps to gnuboy left");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_RIGHT) == 0x01u, "host right maps to gnuboy right");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_UP) == 0x04u, "host up maps to gnuboy up");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_DOWN) == 0x08u, "host down maps to gnuboy down");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_LEFT | MIA_APP_CORE_INPUT_RIGHT) == 0,
                 "contradictory horizontal input is neutral");
    require_true(mia_app_input_gnuboy_mask(MIA_APP_CORE_INPUT_UP | MIA_APP_CORE_INPUT_DOWN) == 0,
                 "contradictory vertical input is neutral");
    require_true(mia_app_input_gw_mask(MIA_APP_CORE_INPUT_UP | MIA_APP_CORE_INPUT_B | MIA_APP_CORE_INPUT_SELECT) == 0x62u, "app buttons map to GW native bits");
    require_true(!mia_app_input_exit_requested(&state, buttons, 1000), "exit combo is debounced initially");
    require_true(mia_app_input_exit_requested(&state, buttons, 1250), "exit combo fires after threshold");
    require_true(!mia_app_input_exit_requested(&state, buttons, 1500), "exit combo is one-shot while held");
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
    std::array<uint8_t, 4> indexed{0, 1, 2, 3};
    std::array<uint16_t, 4> palette{0x0000, 0xf800, 0x07e0, 0x001f};
    std::array<uint16_t, 4> output{};
    require_true(mia_smsplus_convert_frame(indexed.data(), indexed.size(), palette.data(),
                                           palette.size(), output.data(), output.size()),
                 "SMS Plus indexed frame converts");
    require_true(output == palette, "every indexed pixel resolves through the RGB565 palette");
}

static void test_coleco_bios_digest_allowlist_accepts_only_canonical_ntsc() {
    const std::array<uint8_t, MIA_SMSPLUS_SHA1_SIZE> canonical{
        0x45, 0xbe, 0xdc, 0x4c, 0xbd, 0xea, 0xc6, 0x6c, 0x7d, 0xf5,
        0x9e, 0x9e, 0x59, 0x91, 0x95, 0xc7, 0x78, 0xd8, 0x6a, 0x92};
    const std::array<uint8_t, MIA_SMSPLUS_SHA1_SIZE> pal{
        0x16, 0x00, 0x77, 0xaf, 0xb1, 0x39, 0x94, 0x37, 0x25, 0xc6,
        0x34, 0xd6, 0x53, 0x98, 0x98, 0xdb, 0x59, 0xf3, 0x36, 0x57};
    auto mutation = canonical;
    mutation[19] ^= 0x01u;
    const std::array<std::array<uint8_t, MIA_SMSPLUS_SHA1_SIZE>, 4> garbage{{
        {},
        {0x11, 0xf6, 0xad, 0x8e, 0xc5, 0x2a, 0x29, 0x84, 0xab, 0xaa,
         0xfd, 0x7c, 0x3b, 0x51, 0x65, 0x03, 0x78, 0x5c, 0x20, 0x72},
        {0x35, 0xb6, 0x79, 0x5c, 0xa2, 0x0d, 0x6d, 0xc0, 0xaf, 0xf8,
         0xc7, 0xc1, 0x10, 0xc9, 0x6c, 0xd1, 0x07, 0x0b, 0x8c, 0x38},
        {0x5e, 0x2b, 0x96, 0xc1, 0x9c, 0x4f, 0x5c, 0x63, 0xa5, 0xaf,
         0xa2, 0xde, 0x50, 0x4d, 0x29, 0xfe, 0x64, 0xa4, 0xc9, 0x08},
    }};

    require_true(mia_smsplus_validate_coleco_bios_digest(canonical.data(), MIA_SMSPLUS_COLECO_BIOS_SIZE) == MIA_SMSPLUS_BIOS_OK,
                 "canonical NTSC Coleco BIOS digest is accepted");
    require_true(mia_smsplus_validate_coleco_bios_digest(nullptr, MIA_SMSPLUS_COLECO_BIOS_SIZE) == MIA_SMSPLUS_BIOS_MISSING,
                 "missing Coleco BIOS digest is rejected");
    require_true(mia_smsplus_validate_coleco_bios_digest(canonical.data(), MIA_SMSPLUS_COLECO_BIOS_SIZE - 1u) == MIA_SMSPLUS_BIOS_SIZE_INVALID,
                 "short Coleco BIOS is rejected");
    require_true(mia_smsplus_validate_coleco_bios_digest(canonical.data(), MIA_SMSPLUS_COLECO_BIOS_SIZE + 1u) == MIA_SMSPLUS_BIOS_SIZE_INVALID,
                 "oversize Coleco BIOS is rejected");
    require_true(mia_smsplus_validate_coleco_bios_digest(pal.data(), MIA_SMSPLUS_COLECO_BIOS_SIZE) == MIA_SMSPLUS_BIOS_CHECKSUM_INVALID,
                 "PAL Coleco BIOS is rejected");
    require_true(mia_smsplus_validate_coleco_bios_digest(mutation.data(), MIA_SMSPLUS_COLECO_BIOS_SIZE) == MIA_SMSPLUS_BIOS_CHECKSUM_INVALID,
                 "one-bit canonical digest mutation is rejected");
    for (const auto &digest : garbage) {
        require_true(mia_smsplus_validate_coleco_bios_digest(digest.data(), MIA_SMSPLUS_COLECO_BIOS_SIZE) == MIA_SMSPLUS_BIOS_CHECKSUM_INVALID,
                     "single-byte, ASCII, blank, or mostly-FF content digest is rejected");
    }
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
    test_input_maps_host_buttons_and_debounces_exit();
    test_smsplus_maps_target_specific_controls();
    test_smsplus_converts_complete_palette_frame_to_rgb565();
    test_coleco_bios_digest_allowlist_accepts_only_canonical_ntsc();
    test_picker_requires_explicit_entry_and_atomic_save_uses_selected_rom_name();
    return 0;
}
