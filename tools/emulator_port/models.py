from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum, unique
from pathlib import PurePosixPath
from typing import NewType, TypeAlias


AppName = NewType("AppName", str)
CoreFamily = NewType("CoreFamily", str)
LicenseId = NewType("LicenseId", str)
SourcePath = NewType("SourcePath", str)
Sha256 = NewType("Sha256", str)
JsonScalar: TypeAlias = str | int | float | bool | None
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]


@unique
class ManifestCategory(StrEnum):
    EMULATORS = "Emulators"
    GAMES = "Games"


@dataclass(frozen=True, slots=True)
class Geometry:
    width: int
    height: int


@dataclass(frozen=True, slots=True)
class Requirement:
    kind: str
    path: str
    required: bool
    note: str


@dataclass(frozen=True, slots=True)
class Target:
    app_name: AppName
    manifest_category: ManifestCategory
    upstream_namespace: str
    aliases: tuple[str, ...]
    core_family: CoreFamily
    rom_extensions: tuple[str, ...]
    rom_root: PurePosixPath
    save_root: PurePosixPath
    requirements: tuple[Requirement, ...]
    native_geometry: Geometry
    sample_rate_hz: int
    controls: tuple[str, ...]
    source_roots: tuple[SourcePath, ...]
    license_ids: tuple[LicenseId, ...]


@dataclass(frozen=True, slots=True)
class LicenseRecord:
    license_id: LicenseId
    origin_path: SourcePath
    vendor_name: str
    sha256: Sha256
    origin_sha256: Sha256
    source_ref: str | None = None


@dataclass(frozen=True, slots=True)
class ValidationIssue:
    code: str
    detail: str


@dataclass(frozen=True, slots=True)
class ValidationReport:
    issues: tuple[ValidationIssue, ...]
    targets: tuple[Target, ...]

    @property
    def passed(self) -> bool:
        return len(self.issues) == 0
