#!/usr/bin/env python3
"""Build and package distributable MiaOS OTA apps for an SD card."""

from __future__ import annotations

import argparse
import os
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
import subprocess
import time
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

from append_manifest import build_trailer, parse_trailer


ROOT = Path(__file__).resolve().parents[1]
OTA_ROOT = ROOT / "experiments" / "ota_apps"
CATEGORIES = ("Application", "Emulators", "Games", "Media", "Settings", "System", "Utils")
TEXT_LIBRARY = ROOT / "experiments" / "shared_libraries" / "mia_text" / "build" / "libmia_text_v1.so"
TEXT_LIBRARY_ARCHIVE_PATH = "MiaOS/Library/libmia_text_v1.so"
STARTUP_LIBRARY = ROOT / "experiments" / "shared_libraries" / "mia_startup" / "build" / "libmia_startup_v1.so"
STARTUP_LIBRARY_ARCHIVE_PATH = "MiaOS/Library/libmia_startup_v1.so"
SDL_LIBRARY = ROOT / "experiments" / "shared_libraries" / "mia_sdl" / "build" / "libmia_sdl_v1.so"
SDL_LIBRARY_ARCHIVE_PATH = "MiaOS/Library/libmia_sdl_v1.so"


@dataclass(frozen=True)
class App:
    name: str
    category: str
    source: str | None = None

    @property
    def project_dir(self) -> Path:
        return OTA_ROOT / self.name

    @property
    def artifact(self) -> Path:
        if self.source is not None:
            return ROOT / self.source
        return self.project_dir / "build" / f"{self.name}.bin"

    @property
    def archive_path(self) -> str:
        return f"MiaOS/{self.category}/{self.name}.app/{self.name}.bin"


APPS = (
    App("hello", "Application"),
    App("coleco", "Emulators"),
    App("gb", "Emulators"),
    App("gba", "Emulators"),
    App("gbc", "Emulators"),
    App("gg", "Emulators"),
    App("gw", "Emulators"),
    App("lynx", "Emulators"),
    App("megadrive", "Emulators"),
    App("msx", "Emulators"),
    App("nes", "Emulators"),
    App("pce", "Emulators"),
    App("sms", "Emulators"),
    App("snes", "Emulators"),
    App("lava_pal", "Games", "experiments/ota_apps/lavapal/build/lava_pal.bin"),
    App("minesweeper", "Games"),
    App("music", "Media"),
    App("rtc_set", "Settings"),
    App("usb disk", "System", ".pio/build/esp32s3-usbmsc/firmware.bin"),
    App("usb_wifi", "System"),
    App("calculator", "Utils"),
    App("flashlight", "Utils"),
    App("wifi_scan", "Utils"),
    App("diagnostic", "Utils"),
    App("ftp_server", "Utils"),
    App("screen_test", "Utils"),
    App("sd_browser", "Utils"),
    App("timer", "Utils"),
    App("wifi_files", "Utils"),
)


def run(command: list[str]) -> str:
    result = subprocess.run(command, cwd=ROOT, check=True, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return result.stdout.strip()


def version_label() -> str:
    revision = run(["git", "rev-parse", "--short", "HEAD"])
    dirty = subprocess.run(["git", "status", "--porcelain"], cwd=ROOT,
                           check=True, stdout=subprocess.PIPE).stdout
    return f"{revision}-dirty" if dirty else revision


def packaged_firmware(app: App, build_epoch: int) -> bytes:
    data = app.artifact.read_bytes()
    existing = parse_trailer(data)
    if existing:
        data = data[:-existing["size"]]
    packaged = data + build_trailer(app.category, app.name, data, build_epoch)
    manifest = parse_trailer(packaged)
    if not manifest or manifest["category"] != app.category or manifest["name"] != app.name:
        raise ValueError(f"failed to validate manifest for {app.name}")
    return packaged


def zip_info(path: str, build_epoch: int) -> ZipInfo:
    minimum_epoch = 315532800  # ZIP timestamps cannot predate 1980-01-01.
    timestamp = datetime.fromtimestamp(max(build_epoch, minimum_epoch), timezone.utc)
    info = ZipInfo(path, timestamp.timetuple()[:6])
    info.compress_type = ZIP_DEFLATED
    info.external_attr = (0o755 if path.endswith("/") else 0o644) << 16
    return info


def create_archive(output: Path, build_epoch: int) -> tuple[int, int]:
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(f".{output.name}.tmp")
    temporary.unlink(missing_ok=True)
    packed = 0
    skipped = 0
    try:
        with ZipFile(temporary, "w") as archive:
            archive.writestr(zip_info("MiaOS/", build_epoch), b"")
            archive.writestr(zip_info("MiaOS/Library/", build_epoch), b"")
            for category in CATEGORIES:
                archive.writestr(zip_info(f"MiaOS/{category}/", build_epoch), b"")
            if not TEXT_LIBRARY.is_file():
                raise FileNotFoundError(
                    f"missing {TEXT_LIBRARY.relative_to(ROOT)}; build it with "
                    "idf.py -C experiments/shared_libraries/mia_text so"
                )
            library_data = TEXT_LIBRARY.read_bytes()
            archive.writestr(zip_info(TEXT_LIBRARY_ARCHIVE_PATH, build_epoch), library_data)
            print(f"Packed {TEXT_LIBRARY_ARCHIVE_PATH} ({len(library_data)} bytes)")
            packed += 1
            if not STARTUP_LIBRARY.is_file():
                raise FileNotFoundError(
                    f"missing {STARTUP_LIBRARY.relative_to(ROOT)}; build it with "
                    "idf.py -C experiments/shared_libraries/mia_startup so"
                )
            startup_library_data = STARTUP_LIBRARY.read_bytes()
            archive.writestr(zip_info(STARTUP_LIBRARY_ARCHIVE_PATH, build_epoch), startup_library_data)
            print(f"Packed {STARTUP_LIBRARY_ARCHIVE_PATH} ({len(startup_library_data)} bytes)")
            packed += 1
            if not SDL_LIBRARY.is_file():
                raise FileNotFoundError(
                    f"missing {SDL_LIBRARY.relative_to(ROOT)}; build it with "
                    "idf.py -C experiments/shared_libraries/mia_sdl so"
                )
            sdl_library_data = SDL_LIBRARY.read_bytes()
            archive.writestr(zip_info(SDL_LIBRARY_ARCHIVE_PATH, build_epoch), sdl_library_data)
            print(f"Packed {SDL_LIBRARY_ARCHIVE_PATH} ({len(sdl_library_data)} bytes)")
            packed += 1
            for app in APPS:
                if not app.artifact.is_file():
                    print(f"Skipped {app.name}: no {app.artifact.relative_to(ROOT)}")
                    skipped += 1
                    continue
                data = packaged_firmware(app, build_epoch)
                archive.writestr(zip_info(app.archive_path, build_epoch), data)
                print(f"Packed {app.archive_path} ({len(data)} bytes)")
                packed += 1
        temporary.replace(output)
    except Exception:
        temporary.unlink(missing_ok=True)
        raise
    return packed, skipped


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Package existing OTA app artifacts into a /MiaOS SD-card ZIP.")
    parser.add_argument("--output", type=Path,
                        help="output ZIP path (default: dist/<project>_<revision>_sd.zip)")
    parser.add_argument("--build-epoch", type=int,
                        help="manifest/ZIP timestamp (default: SOURCE_DATE_EPOCH or now)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    build_epoch = args.build_epoch
    if build_epoch is None:
        build_epoch = int(os.environ.get("SOURCE_DATE_EPOCH", time.time()))

    output = args.output
    if output is None:
        output = ROOT / "dist" / f"esp32-miaos-pixel_{version_label()}_sd.zip"
    elif not output.is_absolute():
        output = ROOT / output

    packed, skipped = create_archive(output, build_epoch)
    print(f"Created {output.relative_to(ROOT) if output.is_relative_to(ROOT) else output}")
    print(f"Summary: {packed} packed, {skipped} skipped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
