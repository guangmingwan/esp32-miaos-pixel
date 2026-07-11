#include "mia_emulator_core.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" const MiaRuntimeTargetCatalog mia_runtime_generated_targets;

struct FakeHost {
    uint32_t input = 0x51;
    uint32_t video_calls = 0;
    uint32_t audio_calls = 0;
    uint32_t save_calls = 0;
    uint32_t exit_calls = 0;
    std::string save_path;
};

static void require_true(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static const MiaRuntimeTarget *target_named(const char *name) {
    for (size_t index = 0; index < mia_runtime_generated_targets.count; ++index) {
        const MiaRuntimeTarget *target = &mia_runtime_generated_targets.targets[index];
        if (std::strcmp(target->name, name) == 0) {
            return target;
        }
    }
    return nullptr;
}

static uint32_t fake_input(void *ctx) {
    return static_cast<FakeHost *>(ctx)->input;
}

static MiaCoreStatus fake_video(void *ctx, const uint16_t *pixels, size_t pixel_count) {
    require_true(pixels != nullptr, "video callback receives pixels");
    require_true(pixel_count == 4, "video callback receives selected frame size");
    static_cast<FakeHost *>(ctx)->video_calls += 1;
    return mia_core_ok();
}

static MiaCoreStatus fake_audio(void *ctx, const int16_t *frames, size_t frame_count) {
    require_true(frames != nullptr, "audio callback receives frames");
    require_true(frame_count == 2, "audio callback receives selected frame count");
    static_cast<FakeHost *>(ctx)->audio_calls += 1;
    return mia_core_ok();
}

static MiaCoreStatus fake_save(void *ctx, const char *path, const uint8_t *data, size_t size) {
    require_true(data != nullptr, "save callback receives data");
    require_true(size == 3, "save callback receives selected size");
    FakeHost *host = static_cast<FakeHost *>(ctx);
    host->save_calls += 1;
    host->save_path = path;
    return mia_core_ok();
}

static MiaCoreStatus fake_exit(void *ctx) {
    static_cast<FakeHost *>(ctx)->exit_calls += 1;
    return mia_core_ok();
}

static MiaCoreHost host_for(FakeHost *fake) {
    return MiaCoreHost{fake, fake_input, fake_video, fake_audio, fake_save, fake_exit};
}

static void test_target_identity_and_roots() {
    const MiaRuntimeTarget *gb = target_named("gb");
    const MiaRuntimeTarget *gbc = target_named("gbc");
    const MiaRuntimeTarget *gw = target_named("gw");
    require_true(gb != nullptr && gbc != nullptr && gw != nullptr, "gb gbc gw targets exist");
    require_true(std::strcmp(gb->rom_root, "/roms/gb") == 0, "gb rom root is distinct");
    require_true(std::strcmp(gbc->rom_root, "/roms/gbc") == 0, "gbc rom root is distinct");
    require_true(std::strcmp(gw->save_root, "/saves/gw") == 0, "gw save root is distinct");
    require_true(std::strcmp(gb->manifest_category, "Emulators") == 0, "gb category is Emulators");
}

static void test_callbacks_save_and_exit() {
    FakeHost fake;
    MiaCoreAdapter adapter;
    const MiaRuntimeTarget *gb = target_named("gb");
    require_true(mia_core_adapter_init(&adapter, gb, host_for(&fake)).code == MIA_CORE_OK, "adapter initializes");
    require_true(mia_core_adapter_select_rom(&adapter, "/roms/gb/demo.gb", "/saves/gb/demo.sav").code == MIA_CORE_OK, "adapter selects rom");
    require_true(std::strcmp(adapter.rom_path, "/roms/gb/demo.gb") == 0, "adapter retains the explicitly selected ROM path");

    const uint16_t pixels[4] = {0x001f, 0x07e0, 0xf800, 0xffff};
    const int16_t frames[4] = {1, 2, 3, 4};
    const uint8_t save[3] = {9, 8, 7};
    uint32_t input = 0;
    require_true(mia_core_adapter_submit_video(&adapter, pixels, 4).code == MIA_CORE_OK, "video callback submits");
    require_true(mia_core_adapter_submit_audio(&adapter, frames, 2).code == MIA_CORE_OK, "audio callback submits");
    require_true(mia_core_adapter_poll_input(&adapter, &input).code == MIA_CORE_OK, "input callback polls");
    require_true(input == fake.input, "input is returned to core");
    require_true(mia_core_adapter_flush_save(&adapter, save, 3).code == MIA_CORE_OK, "save callback flushes");
    require_true(fake.save_path == "/saves/gb/demo.sav", "save flush uses selected save path");
    require_true(mia_core_adapter_request_exit(&adapter).code == MIA_CORE_OK, "clean exit callback runs");
    require_true(fake.video_calls == 1 && fake.audio_calls == 1 && fake.save_calls == 1 && fake.exit_calls == 1, "callback counts match");
    require_true(adapter.frames_submitted == 1 && adapter.audio_submitted == 2 && adapter.saves_flushed == 1 && adapter.exit_requested == 1, "adapter records observable core activity");
}

int main() {
    test_target_identity_and_roots();
    test_callbacks_save_and_exit();
    return 0;
}
