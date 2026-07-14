from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_lynx_uses_hle_bios_and_core_file_detection() -> None:
    source = (ROOT / "main" / "lynx_adapter.cpp").read_text()
    assert '"/bios/lynxboot.img"' not in source
    assert "HANDY_FILETYPE_ILLEGAL" in source
    assert "mFileType != HANDY_FILETYPE_LNX" not in source
    assert "MIA_LYNX_ERR_HEADER_CORRUPT" in source


def test_lynx_video_controls_and_atomic_eeprom_contract() -> None:
    source = (ROOT / "main" / "lynx_adapter.cpp").read_text()
    cmake = (ROOT / "main" / "CMakeLists.txt").read_text()
    assert "MIKIE_ROTATE_L" in source and "MIKIE_ROTATE_R" in source
    assert "160" in source and "102" in source
    assert "BUTTON_OPT1" in source and "BUTTON_OPT2" in source
    assert "mia_app_save_flush" in source and "MIA_STORAGE_FLUSH_CLEAN_EXIT" in source
    assert "vendor/handy" in cmake and "emulator_common/vendor" not in cmake
    assert (ROOT / "vendor" / "handy" / "license.txt").is_file()
