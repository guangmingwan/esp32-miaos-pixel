from __future__ import annotations

import os
import signal
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, Final

from .inventory import TARGETS
from .models import JsonValue, Target

if TYPE_CHECKING:
    from .suites import SuiteRunOptions, SuiteRunResult

HARDWARE_DIR: Final = Path("experiments/ota_apps/emulator_common/hardware")
BUILD_ROOT: Final = Path(".cache/emulator-port/hardware-adapters")
SANITIZER_FLAGS: Final = "-fsanitize=address,undefined -fno-omit-frame-pointer"
CASE_REGEX: Final[dict[str, str]] = {
    "signals": "hardware_display_tests|hardware_audio_tests|hardware_input_tests",
    "malformed-input": "hardware_malformed_tests",
    "template": "hardware_template_tests",
}


@dataclass(frozen=True, slots=True)
class CStringLiteralError(ValueError):
    value: str
    reason: str

    def __str__(self) -> str:
        return f"invalid C string literal {self.value!r}: {self.reason}"


@dataclass(frozen=True, slots=True)
class NativeCommandResult:
    args: tuple[str, ...]
    returncode: int
    stdout: str
    stderr: str


def run_hardware_adapters_suite(options: "SuiteRunOptions") -> "SuiteRunResult":
    from .suites import SuiteRunResult

    case_issue = case_boundary_issue(options.case)
    if case_issue is not None:
        return SuiteRunResult(1, case_issue)
    root = options.repo_root / BUILD_ROOT / build_name(options.sanitizers)
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f"{case_tag(options.case)}-", dir=root) as build_path:
        build_dir = Path(build_path)
        generated_c = build_dir / "generated" / "mia_hardware_generated_targets.c"
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
        return SuiteRunResult(0, success_report(options, ctest))


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
    args = ["cmake", "-S", str(repo_root / HARDWARE_DIR), "-B", str(build_dir), f"-DMIA_HARDWARE_TARGETS_C={generated_c}"]
    if sanitizers:
        args.extend([f"-DCMAKE_C_FLAGS={SANITIZER_FLAGS}", f"-DCMAKE_CXX_FLAGS={SANITIZER_FLAGS}", f"-DCMAKE_EXE_LINKER_FLAGS={SANITIZER_FLAGS}"])
    return run_command(tuple(args), repo_root)


def run_ctest(repo_root: Path, build_dir: Path, case: str | None) -> NativeCommandResult:
    args = ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
    if case is not None:
        args.extend(["-R", CASE_REGEX[case]])
    return run_command(tuple(args), repo_root)


def run_command(args: tuple[str, ...], cwd: Path) -> NativeCommandResult:
    env = os.environ.copy()
    env["MIA_HARDWARE_REPO_ROOT"] = str(cwd)
    process = subprocess.Popen(args, cwd=cwd, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
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

    return SuiteRunResult(1, {"status": "error", "issue_codes": [f"hardware-adapters-{stage}-failed"], "suite": options.suite, "case": options.case or "all", "command": list(result.args), "returncode": result.returncode, "stdout": result.stdout[-4000:], "stderr": result.stderr[-4000:]})


def success_report(options: "SuiteRunOptions", result: NativeCommandResult) -> dict[str, JsonValue]:
    return {"status": "ok", "suite": options.suite, "case": options.case or "all", "sanitizers": options.sanitizers, "build_retained": False, "target_count": len(TARGETS), "command": list(result.args), "stdout": result.stdout[-4000:]}


def render_target_catalog(targets: tuple[Target, ...]) -> str:
    control_blocks = "\n".join(render_controls(index, target) for index, target in enumerate(targets))
    rows = "\n".join(render_target(index, target) for index, target in enumerate(targets))
    return f'#include "mia_hardware_target.h"\n\n{control_blocks}\n\nstatic const MiaHardwareTarget targets[] = {{\n{rows}\n}};\n\nconst MiaHardwareTargetCatalog mia_hardware_generated_targets = {{targets, {len(targets)}}};\n'


def render_controls(index: int, target: Target) -> str:
    values = ", ".join(c_string(control) for control in target.controls)
    return f"static const char *target_{index}_controls[] = {{{values}}};"


def render_target(index: int, target: Target) -> str:
    return f"    {{{c_string(str(target.app_name))}, {{{target.native_geometry.width}, {target.native_geometry.height}}}, {target.sample_rate_hz}, target_{index}_controls, {len(target.controls)}}},"


def c_string(value: str) -> str:
    encoded: list[str] = []
    for char in value:
        codepoint = ord(char)
        if char == "\0":
            raise CStringLiteralError(value, "NUL byte is not allowed")
        if char == "\n":
            encoded.append("\\n")
        elif char == "\r":
            encoded.append("\\r")
        elif char == "\t":
            encoded.append("\\t")
        elif char == "\\":
            encoded.append("\\\\")
        elif char == '"':
            encoded.append('\\"')
        elif codepoint < 0x20 or codepoint == 0x7F:
            raise CStringLiteralError(value, f"control byte 0x{codepoint:02x} is not allowed")
        elif codepoint > 0x7E:
            raise CStringLiteralError(value, f"non-ASCII codepoint U+{codepoint:04X} is not allowed")
        else:
            encoded.append(char)
    return '"' + "".join(encoded) + '"'
