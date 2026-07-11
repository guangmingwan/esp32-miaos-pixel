from __future__ import annotations

import json
import signal
import subprocess
import time
from collections.abc import Mapping
from pathlib import Path
from typing import TypeAlias

JsonScalar: TypeAlias = str | int | float | bool | None
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]

REPO_ROOT = Path(__file__).resolve().parents[1]
CLI = REPO_ROOT / "tools" / "emulator_port.py"
RUNTIME_CACHE = REPO_ROOT / ".cache" / "emulator-port" / "runtime"


def parse_report(stdout: str) -> Mapping[str, JsonValue]:
    loaded = json.loads(stdout)
    assert isinstance(loaded, dict)
    return loaded


def runtime_process_lines() -> list[str]:
    result = subprocess.run(
        ["pgrep", "-af", r"cmake|gmake|ld|collect2|ctest|runtime_(time|alloc|lifecycle|target)_tests"],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
        timeout=10,
        check=False,
    )
    return [line for line in result.stdout.splitlines() if str(RUNTIME_CACHE) in line]


def generated_dirs() -> set[Path]:
    return set(RUNTIME_CACHE.glob("**/generated"))


def wait_for_new_generated_dir(previous: set[Path]) -> bool:
    deadline = time.monotonic() + 10.0
    while time.monotonic() < deadline:
        if generated_dirs() - previous:
            return True
        time.sleep(0.02)
    return False


def wait_for_runtime_process_cleanup() -> list[str]:
    deadline = time.monotonic() + 5.0
    lines = runtime_process_lines()
    while lines and time.monotonic() < deadline:
        time.sleep(0.05)
        lines = runtime_process_lines()
    return lines


def test_runtime_suite_uses_isolated_build_dirs_for_concurrent_invocations() -> None:
    # Given: two runtime suite cases are launched at the same time.
    lifecycle = subprocess.Popen(
        ["uv", "run", str(CLI), "test", "--suite", "runtime", "--case", "lifecycle"],
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    injected = subprocess.Popen(
        ["uv", "run", str(CLI), "test", "--suite", "runtime", "--case", "injected-failures"],
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    # When: both finish normally.
    lifecycle_stdout, lifecycle_stderr = lifecycle.communicate(timeout=120)
    injected_stdout, injected_stderr = injected.communicate(timeout=120)
    lifecycle_report = parse_report(lifecycle_stdout)
    injected_report = parse_report(injected_stdout)

    # Then: both compiled suites pass and do not share an archive/build tree.
    assert lifecycle.returncode == 0, lifecycle_stdout + lifecycle_stderr
    assert injected.returncode == 0, injected_stdout + injected_stderr
    assert lifecycle_report["status"] == "ok"
    assert injected_report["status"] == "ok"
    assert lifecycle_report["build_dir"] != injected_report["build_dir"]


def test_runtime_suite_direct_sigint_reports_typed_interruption_and_cleans_children() -> None:
    # Given: the runtime CLI has started native setup and owns a runtime build tree.
    before = generated_dirs()
    process = subprocess.Popen(
        ["uv", "run", str(CLI), "test", "--suite", "runtime", "--sanitizers"],
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert wait_for_new_generated_dir(before)

    # When: SIGINT is delivered directly to that CLI process.
    process.send_signal(signal.SIGINT)
    stdout, stderr = process.communicate(timeout=30)
    report = parse_report(stdout)

    # Then: the CLI preserves the typed interruption contract and cleans descendants.
    assert process.returncode == 130, stdout + stderr
    assert report["status"] == "error"
    assert report["issue_codes"] == ["interrupted"]
    assert wait_for_runtime_process_cleanup() == []


def test_runtime_suite_timeout_wrapper_cleans_children_without_requiring_child_stdout() -> None:
    # Given: GNU timeout may interrupt the uv wrapper before child Python emits stdout.
    result = subprocess.run(
        ["timeout", "-s", "INT", "0.01", "uv", "run", str(CLI), "test", "--suite", "runtime", "--sanitizers"],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
        timeout=30,
        check=False,
    )

    # When: the wrapper reports its own timeout status.
    assert result.returncode == 124
    if result.stdout.strip():
        report = parse_report(result.stdout)
        assert report["status"] == "error"
        assert report["issue_codes"] == ["interrupted"]

    # Then: no native descendants remain even when child stdout is empty.
    assert wait_for_runtime_process_cleanup() == []
