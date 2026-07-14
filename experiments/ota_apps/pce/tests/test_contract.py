from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_pce_target_contract() -> None:
    source = (ROOT / "main" / "pce_adapter.c").read_text()
    cmake = (ROOT / "main" / "CMakeLists.txt").read_text()
    assert "LoadFile" in source and "LoadCard" in source and "RunPCE" in source and "psg_update" in source
    assert "mia_app_zip_extract" in source and '".zip"' in source
    assert "display_host_present_rgb565_region" in source
    assert "xTaskCreatePinnedToCore" in source and '"pce_audio"' in source and '"pce_display"' in source
    assert "PCE_AUDIO_CHUNK_FRAMES 62u" in source and "PCE_DISPLAY_BUFFER_COUNT 2u" in source
    assert "return indexed + 16u" in source
    assert "frame->width = frame_width" in source and "display_host_present_rgb565_scaled_region" in source
    assert "mia_app_input_exit_requested" in source
    assert "MIA_STORAGE_FLUSH_CLEAN_EXIT" in source
    assert "vendor/pce-go" in cmake


def test_pce_provenance_and_no_shared_vendor_reference() -> None:
    assert (ROOT / "vendor" / "pce-go" / "COPYING").is_file()
    cmake = (ROOT / "main" / "CMakeLists.txt").read_text()
    assert "emulator_common/vendor" not in cmake
