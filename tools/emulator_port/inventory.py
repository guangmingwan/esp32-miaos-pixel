from __future__ import annotations

from pathlib import PurePosixPath
from typing import Final

from .models import AppName, CoreFamily, Geometry, LicenseId, LicenseRecord, ManifestCategory, Requirement, Sha256, SourcePath, Target


BASE: Final = LicenseId("GPL-2.0-or-later")
BSD_NES: Final = LicenseId("nofrendo-bsd")
BSD_PCE: Final = LicenseId("pce-go-bsd")
BSD_GB: Final = LicenseId("gnuboy-bsd")
BSD_SMS: Final = LicenseId("smsplus-bsd")
SNES9X: Final = LicenseId("snes9x-custom")
GW: Final = LicenseId("gw-emulator")
PRBOOM: Final = LicenseId("prboom-gpl")
GWENESIS: Final = LicenseId("gwenesis")
FMSX: Final = LicenseId("fmsx-noncommercial")


LICENSES: Final[tuple[LicenseRecord, ...]] = (
    LicenseRecord(BASE, SourcePath("COPYING"), "retro-go_COPYING", Sha256("8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643"), Sha256("8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643")),
    LicenseRecord(BSD_NES, SourcePath("retro-core/components/nofrendo/COPYING"), "nofrendo_COPYING", Sha256("91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad"), Sha256("91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad")),
    LicenseRecord(BSD_PCE, SourcePath("retro-core/components/pce-go/COPYING"), "pce-go_COPYING", Sha256("91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad"), Sha256("91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad")),
    LicenseRecord(BSD_GB, SourcePath("retro-core/components/gnuboy/COPYING"), "gnuboy_COPYING", Sha256("91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad"), Sha256("91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad")),
    LicenseRecord(BSD_SMS, SourcePath("retro-core/components/smsplus/COPYING"), "smsplus_COPYING", Sha256("91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad"), Sha256("91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad")),
    LicenseRecord(SNES9X, SourcePath("retro-core/components/snes9x/src/LICENSE"), "snes9x_LICENSE", Sha256("2a92305ffe68c02c084c31638ad5905cc4dd6fcbb480de3503dd192c50f3c04e"), Sha256("2a92305ffe68c02c084c31638ad5905cc4dd6fcbb480de3503dd192c50f3c04e")),
    LicenseRecord(GW, SourcePath("retro-core/components/gw-emulator/LICENSE"), "gw-emulator_LICENSE", Sha256("3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986"), Sha256("3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986")),
    LicenseRecord(PRBOOM, SourcePath("prboom-go/components/prboom/COPYING"), "prboom_COPYING", Sha256("1cba07ce0f6d1366d84b7cc62b76966ba79075e5f379c1e230c3ff0635fa789f"), Sha256("1cba07ce0f6d1366d84b7cc62b76966ba79075e5f379c1e230c3ff0635fa789f")),
    LicenseRecord(GWENESIS, SourcePath("gwenesis/components/gwenesis/LICENSE"), "gwenesis_LICENSE", Sha256("8486a10c4393cee1c25392769ddd3b2d6c242d6ec7928e1414efff7dfb2f07ef"), Sha256("8486a10c4393cee1c25392769ddd3b2d6c242d6ec7928e1414efff7dfb2f07ef")),
    LicenseRecord(FMSX, SourcePath("fmsx/components/fmsx/src/fMSX/fMSX.html"), "fmsx_LICENSE.html", Sha256("81127f0e20e19ec7c4e710fdf7f99990f56df1670873ab39e32f2d47fe5f3b84"), Sha256("81127f0e20e19ec7c4e710fdf7f99990f56df1670873ab39e32f2d47fe5f3b84")),
    LicenseRecord(BASE, SourcePath("gbsp/components/gbsp-libretro/COPYING"), "gbsp_COPYING", Sha256("8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643"), Sha256("8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643"), "2c6e1f22"),
)


def req(kind: str, path: str, required: bool, note: str) -> Requirement:
    return Requirement(kind=kind, path=path, required=required, note=note)


def target(name: str, category: ManifestCategory, namespace: str, aliases: tuple[str, ...], family: str, extensions: tuple[str, ...], geometry: Geometry, sample_rate: int, requirements: tuple[Requirement, ...], source_roots: tuple[str, ...], license_ids: tuple[LicenseId, ...], controls: tuple[str, ...] = ("A", "B", "UP", "DOWN", "LEFT", "RIGHT", "START", "SELECT"), rom_directory: str | None = None) -> Target:
    rom_root = PurePosixPath(f"/roms/{rom_directory or name}")
    return Target(AppName(name), category, namespace, aliases, CoreFamily(family), extensions, rom_root, PurePosixPath(f"/saves/{name}"), requirements, geometry, sample_rate, controls, tuple(SourcePath(root) for root in source_roots), license_ids)


TARGETS: Final[tuple[Target, ...]] = (
    target("nes", ManifestCategory.EMULATORS, "nes", (), "retro-core/nofrendo", ("nes", "fc", "fds", "nsf"), Geometry(256, 240), 32000, (req("bios", "/bios/fds_bios.bin", False, "Required only for FDS images."),), ("retro-core/main/main_nes.c", "retro-core/components/nofrendo"), (BASE, BSD_NES)),
    target("snes", ManifestCategory.EMULATORS, "snes", (), "retro-core/snes9x", ("smc", "sfc"), Geometry(256, 224), 32000, (), ("retro-core/main/main_snes.c", "retro-core/components/snes9x"), (BASE, SNES9X)),
    target("gb", ManifestCategory.EMULATORS, "gb", (), "retro-core/gnuboy", ("gb",), Geometry(160, 144), 32000, (req("bios", "/bios/gb_bios.bin", False, "Optional boot ROM; never bundled."),), ("retro-core/main/main_gbc.c", "retro-core/components/gnuboy"), (BASE, BSD_GB)),
    target("gbc", ManifestCategory.EMULATORS, "gbc", (), "retro-core/gnuboy", ("gbc",), Geometry(160, 144), 32000, (req("bios", "/bios/gbc_bios.bin", False, "Optional boot ROM; never bundled."),), ("retro-core/main/main_gbc.c", "retro-core/components/gnuboy"), (BASE, BSD_GB)),
    target("gw", ManifestCategory.EMULATORS, "gw", (), "retro-core/gw-emulator", ("gw",), Geometry(320, 240), 32000, (), ("retro-core/main/main_gw.c", "retro-core/components/gw-emulator"), (BASE, GW)),
    target("sms", ManifestCategory.EMULATORS, "sms", (), "retro-core/smsplus", ("sms", "sg"), Geometry(256, 192), 32000, (), ("retro-core/main/main_sms.c", "retro-core/components/smsplus"), (BASE, BSD_SMS)),
    target("gg", ManifestCategory.EMULATORS, "gg", (), "retro-core/smsplus", ("gg",), Geometry(160, 144), 32000, (), ("retro-core/main/main_sms.c", "retro-core/components/smsplus"), (BASE, BSD_SMS)),
    target("coleco", ManifestCategory.EMULATORS, "col", ("col",), "retro-core/smsplus", ("col", "rom"), Geometry(256, 192), 32000, (req("bios", "/bios/coleco.rom", True, "ColecoVision BIOS is required and not bundled."),), ("retro-core/main/main_sms.c", "retro-core/components/smsplus"), (BASE, BSD_SMS), rom_directory="col"),
    target("pce", ManifestCategory.EMULATORS, "pce", (), "retro-core/pce-go", ("pce",), Geometry(256, 224), 22050, (), ("retro-core/main/main_pce.c", "retro-core/components/pce-go"), (BASE, BSD_PCE)),
    target("lynx", ManifestCategory.EMULATORS, "lnx", ("lnx",), "retro-core/handy", ("lnx",), Geometry(160, 102), 32000, (req("bios", "/bios/lynxboot.img", True, "Atari Lynx boot ROM is required and not bundled."),), ("retro-core/main/main_lynx.cpp", "retro-core/components/handy"), (BASE,), rom_directory="lnx"),
    target("gba", ManifestCategory.EMULATORS, "gba", (), "gbsp", ("gba",), Geometry(240, 160), 32000, (req("bios", "/bios/gba/gba_bios.bin", True, "GBA BIOS is required and not bundled."),), ("gbsp",), (BASE,)),
    target("megadrive", ManifestCategory.EMULATORS, "gwenesis", ("md", "genesis"), "gwenesis", ("md", "gen", "bin"), Geometry(320, 224), 26633, (), ("gwenesis/main/main.c", "gwenesis/components/gwenesis"), (BASE, GWENESIS), rom_directory="md"),
    target("msx", ManifestCategory.EMULATORS, "msx", (), "fmsx", ("rom", "mx1", "mx2", "dsk"), Geometry(272, 228), 32000, (req("bios", "/bios/msx", True, "MSX BIOS directory is required and not bundled."),), ("fmsx/main/main.c", "fmsx/components/fmsx"), (BASE, FMSX), ("A", "B", "UP", "DOWN", "LEFT", "RIGHT", "START", "SELECT", "KEYBOARD")),
    target("doom", ManifestCategory.GAMES, "doom", (), "prboom-go", ("wad",), Geometry(320, 200), 22050, (req("iwad", "/roms/doom/*.wad", True, "A legal IWAD is required and not bundled."),), ("prboom-go/main", "prboom-go/components/prboom"), (BASE, PRBOOM), ("FIRE", "USE", "STRAFE", "RUN", "UP", "DOWN", "LEFT", "RIGHT", "START", "SELECT")),
)
