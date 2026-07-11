from __future__ import annotations

import tempfile
from pathlib import Path
from typing import TYPE_CHECKING, Final

from .core_adapter_suite import command_failure, run_command

if TYPE_CHECKING:
    from .suites import SuiteRunOptions, SuiteRunResult

CASE_IDS: Final = {"allocation", "save-types"}


def run_gba_suite(options: "SuiteRunOptions") -> "SuiteRunResult":
    from .suites import SuiteRunResult

    if options.case is not None and options.case not in CASE_IDS:
        return SuiteRunResult(1, {"status": "error", "issue_codes": ["unknown-case"], "known_cases": sorted(CASE_IDS)})
    root = options.repo_root / ".cache/emulator-port/gba"
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="gba-", dir=root) as build_path:
        build_dir = Path(build_path)
        source_dir = options.repo_root / "experiments/ota_apps/gba/tests"
        configure = run_command(("cmake", "-S", str(source_dir), "-B", str(build_dir)), options.repo_root)
        if configure.returncode != 0:
            return command_failure("configure", options, configure)
        build = run_command(("cmake", "--build", str(build_dir), "--parallel"), options.repo_root)
        if build.returncode != 0:
            return command_failure("build", options, build)
        ctest = run_command(("ctest", "--test-dir", str(build_dir), "--output-on-failure"), options.repo_root)
        if ctest.returncode != 0:
            return command_failure("ctest", options, ctest)
        return SuiteRunResult(0, {"status": "ok", "target": "gba", "case": options.case or "all", "stdout": ctest.stdout[-4000:]})
