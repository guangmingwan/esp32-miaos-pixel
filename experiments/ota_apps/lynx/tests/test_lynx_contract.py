from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_lynx_bios_and_header_contract() -> None:
    source = (ROOT / "main" / "lynx_adapter.cpp").read_text()
    assert '"/bios/lynxboot.img"' in source
    assert "MIA_LYNX_ERR_BIOS_MISSING" in source
    assert "MIA_LYNX_ERR_BIOS_CORRUPT" in source
    assert "MIA_LYNX_ERR_HEADER_CORRUPT" in source
    assert "LYNX_BOOT_SIZE" in source and "LYNX_BOOT_CRC32" in source


def test_lynx_video_controls_and_atomic_eeprom_contract() -> None:
    source = (ROOT / "main" / "lynx_adapter.cpp").read_text()
    cmake = (ROOT / "main" / "CMakeLists.txt").read_text()
    assert "MIKIE_ROTATE_L" in source and "MIKIE_ROTATE_R" in source
    assert "160" in source and "102" in source
    assert "BUTTON_OPT1" in source and "BUTTON_OPT2" in source
    assert "mia_app_save_flush" in source and "MIA_STORAGE_FLUSH_CLEAN_EXIT" in source
    assert "vendor/handy" in cmake and "emulator_common/vendor" not in cmake
    assert (ROOT / "vendor" / "handy" / "license.txt").is_file()
