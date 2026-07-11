from __future__ import annotations

import importlib.util
import struct
from pathlib import Path
import zlib


SCRIPT = Path(__file__).parents[1] / "tools" / "append_manifest.py"
SPEC = importlib.util.spec_from_file_location("append_manifest", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
append_manifest = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(append_manifest)


def test_mia2_trailer_carries_sortable_version_and_image_identity() -> None:
    image = bytes(range(256)) * 3
    trailer = append_manifest.build_trailer("Emulators", "gbc", image, 1234567890)
    parsed = append_manifest.parse_trailer(image + trailer)

    assert len(trailer) == append_manifest.MIA2_SIZE
    assert parsed == {
        "magic": append_manifest.MIA2_MAGIC,
        "size": append_manifest.MIA2_SIZE,
        "category": "Emulators",
        "name": "gbc",
        "build_epoch": 1234567890,
        "image_size": len(image),
        "image_crc": zlib.crc32(image) & 0xFFFFFFFF,
    }


def test_mia1_trailer_remains_readable() -> None:
    category = b"Emulators".ljust(16, b"\0")
    name = b"gbc".ljust(32, b"\0")
    payload = struct.pack("<I", append_manifest.MIA1_MAGIC) + category + name
    trailer = payload + struct.pack("<I", zlib.crc32(payload) & 0xFFFFFFFF)

    assert append_manifest.parse_trailer(b"firmware" + trailer) == {
        "magic": append_manifest.MIA1_MAGIC,
        "size": append_manifest.MIA1_SIZE,
        "category": "Emulators",
        "name": "gbc",
    }
