from __future__ import annotations

import hashlib
import subprocess
import tarfile
import tempfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Final

from .models import JsonValue

GBA_REF: Final = "2c6e1f22"
GBA_COMMIT: Final = "2c6e1f22e4cec6a2743646244be21b1bc8643c63"
GBA_TREE: Final = "f5866371c5f69552925634977ecceafa53245d6c"
ROOT: Final = PurePosixPath("gbsp/components/gbsp-libretro")
DOCUMENTS: Final = {"COPYING", "README.md", "original_readme.txt"}
HEADERS: Final = {
    ROOT / "libretro/libretro-common/include/boolean.h",
    ROOT / "libretro/libretro-common/include/libretro.h",
}


@dataclass(frozen=True, slots=True)
class VendorOptions:
    source: Path
    source_ref: str
    destination: Path


def run_git(source: Path, *args: str) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(("git", *args), cwd=source, capture_output=True, check=False, timeout=30)


def selected_paths(source: Path) -> tuple[str, ...]:
    result = run_git(source, "ls-tree", "-r", "--name-only", GBA_REF, str(ROOT))
    if result.returncode != 0:
        return ()
    selected: list[str] = []
    for raw in result.stdout.decode().splitlines():
        path = PurePosixPath(raw)
        direct_source = path.parent == ROOT and (path.suffix in {".c", ".cpp", ".h"} or path.name in DOCUMENTS)
        if direct_source or path in HEADERS:
            selected.append(raw)
    return tuple(selected)


def vendor_gba(options: VendorOptions) -> tuple[int, dict[str, JsonValue]]:
    if options.source_ref != GBA_REF:
        return 1, {"status": "error", "issue_codes": ["source-ref-mismatch"], "expected": GBA_REF}
    commit = run_git(options.source, "rev-parse", f"{options.source_ref}^{{commit}}")
    tree = run_git(options.source, "rev-parse", f"{options.source_ref}:gbsp")
    if commit.returncode != 0 or tree.returncode != 0 or commit.stdout.decode().strip() != GBA_COMMIT or tree.stdout.decode().strip() != GBA_TREE:
        return 1, {"status": "error", "issue_codes": ["provenance-mismatch"]}
    paths = selected_paths(options.source)
    if not paths:
        return 1, {"status": "error", "issue_codes": ["empty-vendor-selection"]}
    archive = run_git(options.source, "archive", "--format=tar", options.source_ref, *paths)
    if archive.returncode != 0:
        return 1, {"status": "error", "issue_codes": ["git-archive-failed"]}
    with tempfile.NamedTemporaryFile(suffix=".tar") as handle:
        handle.write(archive.stdout)
        handle.flush()
        with tarfile.open(handle.name, "r:") as bundle:
            bundle.extractall(options.destination, filter="data")
    digest = hashlib.sha256(archive.stdout).hexdigest()
    return 0, {"status": "ok", "target": "gba", "source_ref": GBA_REF, "commit": GBA_COMMIT, "tree": GBA_TREE, "archive_sha256": digest, "file_count": len(paths), "destination": str(options.destination), "excluded_payloads": ["bios/open_gba_bios.bin", "main/bios.h"]}
