from __future__ import annotations

import os
import signal
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, Final

from .inventory import TARGETS
from .models import JsonValue, Requirement, Target

if TYPE_CHECKING:
    from .suites import SuiteRunOptions, SuiteRunResult

STORAGE_DIR: Final = Path("experiments/ota_apps/emulator_common/storage")
BUILD_ROOT: Final = Path(".cache/emulator-port/storage")
SANITIZER_FLAGS: Final = "-fsanitize=address,undefined -fno-omit-frame-pointer"
CASE_REGEX: Final[dict[str, str]] = {
    "picker-save": "storage_picker_tests|storage_save_tests",
    "invalid-storage": "storage_invalid_tests",
}


@dataclass(frozen=True, slots=True)
class NativeCommandResult:
    args: tuple[str, ...]
    returncode: int
    stdout: str
    stderr: str


def run_storage_suite(options: "SuiteRunOptions") -> "SuiteRunResult":
    from .suites import SuiteRunResult

    case_issue = case_boundary_issue(options.case)
    if case_issue is not None:
        return SuiteRunResult(1, case_issue)
    root = options.repo_root / BUILD_ROOT / build_name(options.sanitizers)
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f"{case_tag(options.case)}-", dir=root) as build_path:
        build_dir = Path(build_path)
        generated_c = build_dir / "generated" / "mia_storage_generated_targets.c"
        generated_c.parent.mkdir(parents=True, exist_ok=True)
        generated_c.write_text(render_target_catalog(TARGETS), encoding="utf-8")
        configure = run_configure(options.repo_root, build_dir, generated_c, options.sanitizers)
        if configure.returncode != 0:
            return command_failure("configure", options, configure)
        build = run_command(("cmake", "--build", str(build_dir), "--parallel"), options.repo_root)
        if build.returncode != 0:
            return command_failure("build", options, build)
        ctest = run_ctest(options.repo_root, build_dir, options.case)
        if ctest.returncode != 0:
            return command_failure("ctest", options, ctest)
        return SuiteRunResult(0, success_report(options, build_dir, ctest))


def case_boundary_issue(case: str | None) -> dict[str, JsonValue] | None:
    if case is None or case in CASE_REGEX:
        return None
    return {"status": "error", "issue_codes": ["unknown-case"], "case": case, "known_cases": sorted(CASE_REGEX)}


def build_name(sanitizers: bool) -> str:
    return "sanitizers" if sanitizers else "default"


def case_tag(case: str | None) -> str:
    if case is None:
        return "all"
    return "".join(char if char.isalnum() else "-" for char in case)


def run_configure(repo_root: Path, build_dir: Path, generated_c: Path, sanitizers: bool) -> NativeCommandResult:
    args = ["cmake", "-S", str(repo_root / STORAGE_DIR), "-B", str(build_dir), f"-DMIA_STORAGE_TARGETS_C={generated_c}"]
    if sanitizers:
        args.extend([f"-DCMAKE_C_FLAGS={SANITIZER_FLAGS}", f"-DCMAKE_CXX_FLAGS={SANITIZER_FLAGS}", f"-DCMAKE_EXE_LINKER_FLAGS={SANITIZER_FLAGS}"])
    return run_command(tuple(args), repo_root)


def run_ctest(repo_root: Path, build_dir: Path, case: str | None) -> NativeCommandResult:
    args = ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
    if case is not None:
        args.extend(["-R", CASE_REGEX[case]])
    return run_command(tuple(args), repo_root)


def run_command(args: tuple[str, ...], cwd: Path) -> NativeCommandResult:
    process = subprocess.Popen(args, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try:
        stdout, stderr = process.communicate(timeout=120)
    except KeyboardInterrupt:
        terminate_process_group(process)
        raise
    except subprocess.TimeoutExpired:
        terminate_process_group(process)
        stdout, stderr = process.communicate(timeout=5)
        return NativeCommandResult(args=args, returncode=124, stdout=stdout, stderr=stderr)
    return NativeCommandResult(args=args, returncode=process.returncode, stdout=stdout, stderr=stderr)


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


def command_failure(stage: str, options: "SuiteRunOptions", result: NativeCommandResult) -> "SuiteRunResult":
    from .suites import SuiteRunResult

    return SuiteRunResult(1, {"status": "error", "issue_codes": [f"storage-{stage}-failed"], "suite": options.suite, "case": options.case or "all", "command": list(result.args), "returncode": result.returncode, "stdout": result.stdout[-4000:], "stderr": result.stderr[-4000:]})


def success_report(options: "SuiteRunOptions", build_dir: Path, result: NativeCommandResult) -> dict[str, JsonValue]:
    return {"status": "ok", "suite": options.suite, "case": options.case or "all", "sanitizers": options.sanitizers, "build_dir": str(build_dir), "target_count": len(TARGETS), "command": list(result.args), "stdout": result.stdout[-4000:]}


def render_target_catalog(targets: tuple[Target, ...]) -> str:
    rows = "\n".join(render_target(index, target) for index, target in enumerate(targets))
    extras = "\n".join(render_target_extras(index, target) for index, target in enumerate(targets))
    return f'#include "mia_storage.h"\n\n{extras}\n\nstatic const MiaStorageTarget targets[] = {{\n{rows}\n}};\n\nconst MiaStorageTargetCatalog mia_storage_generated_targets = {{targets, {len(targets)}}};\n'


def render_target_extras(index: int, target: Target) -> str:
    extensions = ", ".join(c_string(extension) for extension in target.rom_extensions)
    if len(target.requirements) == 0:
        return f"static const char *target_{index}_extensions[] = {{{extensions}}};"
    requirements = "\n".join(render_requirement(requirement) for requirement in target.requirements)
    return f"static const char *target_{index}_extensions[] = {{{extensions}}};\nstatic const MiaStorageRequirement target_{index}_requirements[] = {{\n{requirements}\n}};"


def render_requirement(requirement: Requirement) -> str:
    return f"    {{{c_string(requirement.kind)}, {c_string(requirement.path)}, {str(requirement.required).lower()}}},"


def render_target(index: int, target: Target) -> str:
    requirements = f"target_{index}_requirements" if len(target.requirements) > 0 else "NULL"
    return f"    {{{c_string(str(target.app_name))}, {c_string(str(target.rom_root))}, {c_string(str(target.save_root))}, target_{index}_extensions, {len(target.rom_extensions)}, {requirements}, {len(target.requirements)}}},"


def c_string(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'
