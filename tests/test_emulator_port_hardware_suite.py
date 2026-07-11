from __future__ import annotations

import json
import subprocess
from dataclasses import replace
from pathlib import Path
import sys
from collections.abc import Mapping
from typing import TypeAlias

import pytest

JsonScalar: TypeAlias = str | int | float | bool | None
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]
REPO_ROOT = Path(__file__).resolve().parents[1]
CLI = REPO_ROOT / "tools" / "emulator_port.py"
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))


def parse_report(stdout: str) -> Mapping[str, JsonValue]:
    loaded = json.loads(stdout)
    assert isinstance(loaded, dict)
    return loaded


def test_hardware_suite_escapes_control_characters_in_generated_c() -> None:
    from emulator_port.hardware_adapters_suite import render_target_catalog
    from emulator_port.inventory import TARGETS

    target = replace(TARGETS[0], controls=("LINE\nTAB\tCR\rQUOTE\"SLASH\\",))

    generated = render_target_catalog((target,))

    assert '"LINE\\nTAB\\tCR\\rQUOTE\\"SLASH\\\\"' in generated
    assert '"LINE\nTAB' not in generated


def test_hardware_suite_rejects_nul_in_generated_c_literal() -> None:
    from emulator_port.hardware_adapters_suite import render_target_catalog
    from emulator_port.inventory import TARGETS

    target = replace(TARGETS[0], controls=("BAD\x00CONTROL",))

    with pytest.raises(ValueError, match="NUL"):
        render_target_catalog((target,))


def test_hardware_suite_unknown_case_lists_template() -> None:
    from emulator_port.hardware_adapters_suite import case_boundary_issue

    report = case_boundary_issue("not-real")

    assert report is not None
    assert report["known_cases"] == ["malformed-input", "signals", "template"]


def test_hardware_suite_success_report_does_not_advertise_removed_build_dir() -> None:
    result = subprocess.run(
        ["uv", "run", str(CLI), "test", "--suite", "hardware-adapters", "--case", "template"],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
        timeout=120,
        check=False,
    )
    report = parse_report(result.stdout)

    assert result.returncode == 0, result.stdout + result.stderr
    assert report["status"] == "ok"
    assert report["build_retained"] is False
    assert "build_dir" not in report


def test_todo5_gnuboy_uses_cpu_native_rgb565() -> None:
    source = (
        REPO_ROOT
        / "experiments"
        / "ota_apps"
        / "emulator_common"
        / "app"
        / "src"
        / "mia_emulator_gnuboy.c"
    ).read_text(encoding="utf-8")

    assert "GB_PIXEL_565_LE" in source
    assert "GB_PIXEL_565_BE" not in source
