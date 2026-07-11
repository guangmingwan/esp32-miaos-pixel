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

CORE_DIR: Final = Path("experiments/ota_apps/emulator_common/core")
BUILD_ROOT: Final = Path(".cache/emulator-port/core-adapter")
CASE_REGEX: Final[dict[str, str]] = {
    "identity": "core_adapter_tests",
    "callbacks": "core_adapter_tests",
}


@dataclass(frozen=True, slots=True)
class NativeCommandResult:
    args: tuple[str, ...]
    returncode: int
    stdout: str
    stderr: str


def run_core_adapter_suite(options: "SuiteRunOptions") -> "SuiteRunResult":
    from .suites import SuiteRunResult

    case_issue = case_boundary_issue(options.case)
    if case_issue is not None:
        return SuiteRunResult(1, case_issue)
    root = options.repo_root / BUILD_ROOT / ("sanitizers" if options.sanitizers else "default")
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="core-", dir=root) as build_path:
        build_dir = Path(build_path)
        generated_c = build_dir / "generated" / "mia_core_generated_targets.c"
        generated_c.parent.mkdir(parents=True, exist_ok=True)
        generated_c.write_text(render_target_catalog(TARGETS), encoding="utf-8")
        configure = run_command(("cmake", "-S", str(options.repo_root / CORE_DIR), "-B", str(build_dir), f"-DMIA_CORE_TARGETS_C={generated_c}"), options.repo_root)
        if configure.returncode != 0:
            return command_failure("configure", options, configure)
        build = run_command(("cmake", "--build", str(build_dir), "--parallel"), options.repo_root)
        if build.returncode != 0:
            return command_failure("build", options, build)
        ctest = run_command(("ctest", "--test-dir", str(build_dir), "--output-on-failure"), options.repo_root)
        if ctest.returncode != 0:
            return command_failure("ctest", options, ctest)
        return SuiteRunResult(0, {"status": "ok", "suite": options.suite, "case": options.case or "all", "target_count": len(TARGETS), "stdout": ctest.stdout[-4000:]})


def case_boundary_issue(case: str | None) -> dict[str, JsonValue] | None:
    if case is None or case in CASE_REGEX:
        return None
    return {"status": "error", "issue_codes": ["unknown-case"], "case": case, "known_cases": sorted(CASE_REGEX)}


def run_command(args: tuple[str, ...], cwd: Path) -> NativeCommandResult:
    process = subprocess.Popen(args, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try:
        stdout, stderr = process.communicate(timeout=120)
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

    return SuiteRunResult(1, {"status": "error", "issue_codes": [f"core-adapter-{stage}-failed"], "suite": options.suite, "case": options.case or "all", "command": list(result.args), "returncode": result.returncode, "stdout": result.stdout[-4000:], "stderr": result.stderr[-4000:]})


def render_target_catalog(targets: tuple[Target, ...]) -> str:
    aliases = "\n".join(render_aliases(index, target) for index, target in enumerate(targets))
    rows = "\n".join(render_target(target) for target in targets)
    return f'#include "mia_runtime_target.h"\n\n{aliases}\n\nstatic const MiaRuntimeTarget targets[] = {{\n{rows}\n}};\n\nconst MiaRuntimeTargetCatalog mia_runtime_generated_targets = {{targets, {len(targets)}}};\n'


def render_aliases(index: int, target: Target) -> str:
    values = ", ".join(c_string(alias) for alias in target.aliases)
    return f"static const char *target_{index}_aliases[] = {{{values}}};"


def render_target(target: Target) -> str:
    return f'    {{{c_string(str(target.app_name))}, {c_string(target.manifest_category.value)}, {c_string(target.upstream_namespace)}, {c_string(str(target.core_family))}, {c_string(str(target.rom_root))}, {c_string(str(target.save_root))}, {{{target.native_geometry.width}, {target.native_geometry.height}}}, {target.sample_rate_hz}, target_{TARGETS.index(target)}_aliases, {len(target.aliases)}}},'


def c_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'
