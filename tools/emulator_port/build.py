from __future__ import annotations

import hashlib
import os
import signal
import shutil
import struct
import subprocess
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Final

from .inventory import TARGETS
from .models import JsonValue, Target

MANIFEST_MAGIC: Final = 0x3141494D
MANIFEST_SIZE: Final = 56
IDF_EXPORT: Final = Path("/opt/esp-idf/export.sh")
APP_ROOT: Final = Path("experiments/ota_apps")


@dataclass(frozen=True, slots=True)
class BuildOptions:
    repo_root: Path
    target_names: tuple[str, ...]
    append_manifest: bool
    size_limit: int


@dataclass(frozen=True, slots=True)
class CommandResult:
    args: tuple[str, ...]
    returncode: int
    stdout: str
    stderr: str


@dataclass(frozen=True, slots=True)
class Manifest:
    category: str
    name: str
    crc: int


def run_build(options: BuildOptions) -> tuple[int, dict[str, JsonValue]]:
    targets_or_error = resolve_targets(options.target_names)
    if isinstance(targets_or_error, dict):
        return 1, targets_or_error
    artifacts: list[JsonValue] = []
    for target in targets_or_error:
        issue = project_issue(options.repo_root, target)
        if issue is not None:
            return 1, issue
        result = build_target(options.repo_root, target)
        if result.returncode != 0:
            return 1, command_error("idf-build-failed", target, result)
        artifact_or_error = finalize_artifact(options.repo_root, target, options)
        if isinstance(artifact_or_error, dict) and artifact_or_error.get("status") == "error":
            return 1, artifact_or_error
        artifacts.append(artifact_or_error)
    return 0, {"status": "ok", "targets": list(options.target_names), "artifact_count": len(artifacts), "artifacts": artifacts}


def resolve_targets(names: tuple[str, ...]) -> tuple[Target, ...] | dict[str, JsonValue]:
    if len(names) == 0:
        return {"status": "error", "issue_codes": ["missing-targets"]}
    known = {str(target.app_name): target for target in TARGETS}
    seen: set[str] = set()
    selected: list[Target] = []
    for name in names:
        if name in seen:
            return {"status": "error", "issue_codes": ["duplicate-target"], "target": name}
        seen.add(name)
        target = known.get(name)
        if target is None:
            return {"status": "error", "issue_codes": ["unknown-target"], "target": name, "known_targets": sorted(known)}
        selected.append(target)
    return tuple(selected)


def project_issue(repo_root: Path, target: Target) -> dict[str, JsonValue] | None:
    project_dir = repo_root / APP_ROOT / str(target.app_name)
    if not project_dir.is_dir():
        return {"status": "error", "issue_codes": ["missing-project"], "target": str(target.app_name), "project": str(project_dir)}
    if not (project_dir / "CMakeLists.txt").is_file():
        return {"status": "error", "issue_codes": ["missing-project-cmake"], "target": str(target.app_name), "project": str(project_dir)}
    return None


def build_target(repo_root: Path, target: Target) -> CommandResult:
    if not IDF_EXPORT.is_file():
        return CommandResult(args=(str(IDF_EXPORT),), returncode=127, stdout="", stderr="ESP-IDF export.sh not found")
    build_dir = repo_root / APP_ROOT / str(target.app_name) / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)
    project_dir = repo_root / APP_ROOT / str(target.app_name)
    for generated_name in ("sdkconfig", "sdkconfig.old"):
        generated = project_dir / generated_name
        if generated.exists():
            generated.unlink()
    command = f"source {IDF_EXPORT} >/dev/null && idf.py build -C {APP_ROOT / str(target.app_name)}"
    return run_command(("bash", "-lc", command), repo_root, 600)


def run_command(args: tuple[str, ...], cwd: Path, timeout_seconds: int) -> CommandResult:
    process = subprocess.Popen(args, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try:
        stdout, stderr = process.communicate(timeout=timeout_seconds)
    except subprocess.TimeoutExpired:
        terminate_process_group(process)
        stdout, stderr = process.communicate(timeout=5)
        return CommandResult(args=args, returncode=124, stdout=stdout, stderr=stderr)
    return CommandResult(args=args, returncode=process.returncode, stdout=stdout, stderr=stderr)


def terminate_process_group(process: subprocess.Popen[str]) -> None:
    try:
        pgid = os.getpgid(process.pid)
    except ProcessLookupError:
        return
    os.killpg(pgid, signal.SIGTERM)
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        os.killpg(pgid, signal.SIGKILL)
        process.wait(timeout=5)


def command_error(code: str, target: Target, result: CommandResult) -> dict[str, JsonValue]:
    return {"status": "error", "issue_codes": [code], "target": str(target.app_name), "command": list(result.args), "returncode": result.returncode, "stdout": result.stdout[-4000:], "stderr": result.stderr[-4000:]}


def artifact_path(repo_root: Path, target: Target) -> Path:
    name = str(target.app_name)
    return repo_root / APP_ROOT / name / "build" / f"{name}.bin"


def finalize_artifact(repo_root: Path, target: Target, options: BuildOptions) -> dict[str, JsonValue]:
    path = artifact_path(repo_root, target)
    if not path.is_file():
        return {"status": "error", "issue_codes": ["missing-artifact"], "target": str(target.app_name), "artifact": str(path)}
    data = path.read_bytes()
    if parse_manifest(data) is not None:
        data = data[:-MANIFEST_SIZE]
    if options.append_manifest:
        data = data + build_trailer(target.manifest_category.value, str(target.app_name))
        path.write_bytes(data)
    manifest = parse_manifest(data)
    if options.append_manifest and manifest is None:
        return {"status": "error", "issue_codes": ["manifest-parse-failed"], "target": str(target.app_name), "artifact": str(path)}
    size = len(data)
    if size > options.size_limit:
        return {"status": "error", "issue_codes": ["oversize"], "target": str(target.app_name), "artifact": str(path), "size": size, "size_limit": options.size_limit}
    manifest_json: JsonValue = None
    if manifest is not None:
        manifest_json = {"category": manifest.category, "name": manifest.name, "crc": manifest.crc}
    return {"target": str(target.app_name), "category": target.manifest_category.value, "rom_root": str(target.rom_root), "save_root": str(target.save_root), "artifact": str(path), "size": size, "sha256": hashlib.sha256(data).hexdigest(), "manifest": manifest_json}


def build_trailer(category: str, name: str) -> bytes:
    category_bytes = category.encode("utf-8").ljust(16, b"\0")[:16]
    name_bytes = name.encode("utf-8").ljust(32, b"\0")[:32]
    crc = zlib.crc32(struct.pack("<I", MANIFEST_MAGIC) + category_bytes + name_bytes) & 0xFFFFFFFF
    return struct.pack("<I", MANIFEST_MAGIC) + category_bytes + name_bytes + struct.pack("<I", crc)


def parse_manifest(data: bytes) -> Manifest | None:
    if len(data) < MANIFEST_SIZE:
        return None
    trailer = data[-MANIFEST_SIZE:]
    magic = struct.unpack_from("<I", trailer, 0)[0]
    if magic != MANIFEST_MAGIC:
        return None
    category_bytes = trailer[4:20]
    name_bytes = trailer[20:52]
    stored_crc = struct.unpack_from("<I", trailer, 52)[0]
    expected_crc = zlib.crc32(struct.pack("<I", magic) + category_bytes + name_bytes) & 0xFFFFFFFF
    if stored_crc != expected_crc:
        return None
    return Manifest(category_bytes.rstrip(b"\0").decode("utf-8"), name_bytes.rstrip(b"\0").decode("utf-8"), stored_crc)
