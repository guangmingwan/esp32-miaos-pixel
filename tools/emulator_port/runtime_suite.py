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

RUNTIME_DIR: Final = Path("experiments/ota_apps/emulator_common/runtime")
BUILD_ROOT: Final = Path(".cache/emulator-port/runtime")
SANITIZER_FLAGS: Final = "-fsanitize=address,undefined -fno-omit-frame-pointer"
CASE_REGEX: Final[dict[str, str]] = {
    "lifecycle": "runtime_lifecycle_tests",
    "injected-failures": "runtime_alloc_tests|runtime_time_tests|runtime_lifecycle_tests",
}


@dataclass(frozen=True, slots=True)
class NativeCommandResult:
    args: tuple[str, ...]
    returncode: int
    stdout: str
    stderr: str


def run_runtime_suite(options: "SuiteRunOptions") -> "SuiteRunResult":
    from .suites import SuiteRunResult

    case_issue = case_boundary_issue(options.case)
    if case_issue is not None:
        return SuiteRunResult(1, case_issue)
    root = options.repo_root / BUILD_ROOT / build_name(options.sanitizers)
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f"{case_tag(options.case)}-", dir=root) as build_path:
        build_dir = Path(build_path)
        generated_c = build_dir / "generated" / "mia_runtime_generated_targets.c"
        generated_c.parent.mkdir(parents=True, exist_ok=True)
        generated_c.write_text(render_target_catalog(TARGETS), encoding="utf-8")

        configure = run_configure(options.repo_root, build_dir, generated_c, options.sanitizers)
        if configure.returncode != 0:
            return command_failure("configure", options, configure)
        build = run_command(("cmake", "--build", str(build_dir), "--parallel", "1"), options.repo_root)
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
    if sanitizers:
        return "sanitizers"
    return "default"


def case_tag(case: str | None) -> str:
    if case is None:
        return "all"
    return "".join(char if char.isalnum() else "-" for char in case)


def run_configure(repo_root: Path, build_dir: Path, generated_c: Path, sanitizers: bool) -> NativeCommandResult:
    args = ["cmake", "-S", str(repo_root / RUNTIME_DIR), "-B", str(build_dir), f"-DMIA_RUNTIME_TARGETS_C={generated_c}"]
    if sanitizers:
        args.extend([f"-DCMAKE_C_FLAGS={SANITIZER_FLAGS}", f"-DCMAKE_CXX_FLAGS={SANITIZER_FLAGS}", f"-DCMAKE_EXE_LINKER_FLAGS={SANITIZER_FLAGS}"])
    return run_command(tuple(args), repo_root)


def run_ctest(repo_root: Path, build_dir: Path, case: str | None) -> NativeCommandResult:
    args = ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
    if case is not None:
        args.extend(["-R", CASE_REGEX[case]])
    return run_command(tuple(args), repo_root)


def run_command(args: tuple[str, ...], cwd: Path) -> NativeCommandResult:
    process = subprocess.Popen(
        args,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        start_new_session=True,
    )
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

    return SuiteRunResult(
        1,
        {
            "status": "error",
            "issue_codes": [f"runtime-{stage}-failed"],
            "suite": options.suite,
            "case": options.case or "all",
            "command": list(result.args),
            "returncode": result.returncode,
            "stdout": result.stdout[-4000:],
            "stderr": result.stderr[-4000:],
        },
    )


def success_report(options: "SuiteRunOptions", build_dir: Path, result: NativeCommandResult) -> dict[str, JsonValue]:
    return {
        "status": "ok",
        "suite": options.suite,
        "case": options.case or "all",
        "sanitizers": options.sanitizers,
        "build_dir": str(build_dir),
        "target_count": len(TARGETS),
        "command": list(result.args),
        "stdout": result.stdout[-4000:],
    }


def render_target_catalog(targets: tuple[Target, ...]) -> str:
    alias_blocks = "\n".join(render_aliases(index, target) for index, target in enumerate(targets))
    target_rows = "\n".join(render_target(index, target) for index, target in enumerate(targets))
    return f'#include "mia_runtime_target.h"\n\n{alias_blocks}\n\nstatic const MiaRuntimeTarget targets[] = {{\n{target_rows}\n}};\n\nconst MiaRuntimeTargetCatalog mia_runtime_generated_targets = {{targets, {len(targets)}}};\n'


def render_aliases(index: int, target: Target) -> str:
    if len(target.aliases) == 0:
        return f"static const char *const aliases_{index}[] = {{0}};"
    values = ", ".join(c_string(alias) for alias in target.aliases)
    return f"static const char *const aliases_{index}[] = {{{values}}};"


def render_target(index: int, target: Target) -> str:
    return (
        "    {"
        f"{c_string(str(target.app_name))}, {c_string(target.manifest_category.value)}, "
        f"{c_string(target.upstream_namespace)}, {c_string(str(target.core_family))}, "
        f"{c_string(str(target.rom_root))}, {c_string(str(target.save_root))}, "
        f"{{{target.native_geometry.width}, {target.native_geometry.height}}}, {target.sample_rate_hz}, "
        f"aliases_{index}, {len(target.aliases)}"
        "},"
    )


def c_string(value: str) -> str:
    return '"' + value.replace('\\', '\\\\').replace('"', '\\"') + '"'
