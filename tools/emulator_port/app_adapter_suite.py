from __future__ import annotations

import tempfile
from pathlib import Path
from typing import TYPE_CHECKING, Final

from .core_adapter_suite import command_failure, run_command
from .inventory import TARGETS

if TYPE_CHECKING:
    from .suites import SuiteRunOptions, SuiteRunResult

APP_DIR: Final = Path("experiments/ota_apps/emulator_common/app")
BUILD_ROOT: Final = Path(".cache/emulator-port/app-adapter")
CASE_IDS: Final = {"picker", "video", "audio", "input", "save"}


def run_app_adapter_suite(options: "SuiteRunOptions") -> "SuiteRunResult":
    from .suites import SuiteRunResult

    if options.case is not None and options.case not in CASE_IDS:
        return SuiteRunResult(1, {"status": "error", "issue_codes": ["unknown-case"], "case": options.case, "known_cases": sorted(CASE_IDS)})
    root = options.repo_root / BUILD_ROOT / ("sanitizers" if options.sanitizers else "default")
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="app-", dir=root) as build_path:
        build_dir = Path(build_path)
        generated_c = build_dir / "generated" / "mia_app_generated_targets.c"
        generated_c.parent.mkdir(parents=True, exist_ok=True)
        generated_c.write_text(render_app_catalogs(), encoding="utf-8")
        configure = run_command(("cmake", "-S", str(options.repo_root / APP_DIR), "-B", str(build_dir), f"-DMIA_APP_TARGETS_C={generated_c}"), options.repo_root)
        if configure.returncode != 0:
            return command_failure("configure", options, configure)
        build = run_command(("cmake", "--build", str(build_dir), "--parallel"), options.repo_root)
        if build.returncode != 0:
            return command_failure("build", options, build)
        ctest = run_command(("ctest", "--test-dir", str(build_dir), "--output-on-failure"), options.repo_root)
        if ctest.returncode != 0:
            return command_failure("ctest", options, ctest)
        return SuiteRunResult(0, {"status": "ok", "suite": options.suite, "case": options.case or "all", "target_count": len(TARGETS), "stdout": ctest.stdout[-4000:]})


def render_app_catalogs() -> str:
    aliases = "\n".join(render_items("aliases", index, target.aliases) for index, target in enumerate(TARGETS))
    controls = "\n".join(render_items("controls", index, target.controls) for index, target in enumerate(TARGETS))
    extensions = "\n".join(render_items("extensions", index, target.rom_extensions) for index, target in enumerate(TARGETS))
    requirements = "\n".join(render_requirements(index) for index, _target in enumerate(TARGETS))
    runtime_rows = "\n".join(render_runtime_target(index) for index, _target in enumerate(TARGETS))
    hardware_rows = "\n".join(render_hardware_target(index) for index, _target in enumerate(TARGETS))
    storage_rows = "\n".join(render_storage_target(index) for index, _target in enumerate(TARGETS))
    return f'#include "mia_hardware_target.h"\n#include "mia_runtime_target.h"\n#include "mia_storage.h"\n\n{aliases}\n{controls}\n{extensions}\n{requirements}\n\nstatic const MiaRuntimeTarget runtime_targets[] = {{\n{runtime_rows}\n}};\nconst MiaRuntimeTargetCatalog mia_runtime_generated_targets = {{runtime_targets, {len(TARGETS)}}};\n\nstatic const MiaHardwareTarget hardware_targets[] = {{\n{hardware_rows}\n}};\nconst MiaHardwareTargetCatalog mia_hardware_generated_targets = {{hardware_targets, {len(TARGETS)}}};\n\nstatic const MiaStorageTarget storage_targets[] = {{\n{storage_rows}\n}};\nconst MiaStorageTargetCatalog mia_storage_generated_targets = {{storage_targets, {len(TARGETS)}}};\n'


def render_items(kind: str, index: int, values: tuple[str, ...]) -> str:
    items = ", ".join(c_string(value) for value in values)
    return f"static const char *target_{index}_{kind}[] = {{{items}}};"


def render_requirements(index: int) -> str:
    target = TARGETS[index]
    if not target.requirements:
        return f"static const MiaStorageRequirement target_{index}_requirements[] = {{0}};"
    rows = ", ".join(f"{{{c_string(req.kind)}, {c_string(req.path)}, {str(req.required).lower()}}}" for req in target.requirements)
    return f"static const MiaStorageRequirement target_{index}_requirements[] = {{{rows}}};"


def render_runtime_target(index: int) -> str:
    target = TARGETS[index]
    return f'    {{{c_string(str(target.app_name))}, {c_string(target.manifest_category.value)}, {c_string(target.upstream_namespace)}, {c_string(str(target.core_family))}, {c_string(str(target.rom_root))}, {c_string(str(target.save_root))}, {{{target.native_geometry.width}, {target.native_geometry.height}}}, {target.sample_rate_hz}, target_{index}_aliases, {len(target.aliases)}}},'


def render_hardware_target(index: int) -> str:
    target = TARGETS[index]
    return f'    {{{c_string(str(target.app_name))}, {{{target.native_geometry.width}, {target.native_geometry.height}}}, {target.sample_rate_hz}, target_{index}_controls, {len(target.controls)}}},'


def render_storage_target(index: int) -> str:
    target = TARGETS[index]
    return f'    {{{c_string(str(target.app_name))}, {c_string(str(target.rom_root))}, {c_string(str(target.save_root))}, target_{index}_extensions, {len(target.rom_extensions)}, target_{index}_requirements, {len(target.requirements)}, NULL, 0}},'


def c_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'
