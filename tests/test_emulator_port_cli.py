from __future__ import annotations

import json
import shutil
import subprocess
import sys
from collections.abc import Mapping
from dataclasses import replace
from pathlib import Path
from typing import TypeAlias


JsonScalar: TypeAlias = str | int | float | bool | None
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]

REPO_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = Path("/home/netwan/myprojects/esp32-retro-go-pixel")
CLI = REPO_ROOT / "tools" / "emulator_port.py"
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))


def run_cli(*args: str, timeout: int = 30) -> subprocess.CompletedProcess[str]:
    # Given: the emulator_port CLI is invoked through its executable surface.
    # When: the command runs with bounded execution.
    # Then: the caller can assert on observable stdout/stderr/exit status.
    return subprocess.run(
        ["uv", "run", str(CLI), *args],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
        timeout=timeout,
        check=False,
    )


def parse_report(stdout: str) -> Mapping[str, JsonValue]:
    loaded = json.loads(stdout)
    assert isinstance(loaded, dict)
    return loaded


def test_validate_passes_for_upstream_inventory() -> None:
    result = run_cli("validate", "--source", str(SOURCE_ROOT))
    report = parse_report(result.stdout)

    assert result.returncode == 0, result.stderr
    assert report["status"] == "ok"
    assert report["target_count"] == 14
    assert report["categories"] == {"Games/doom": 1, "Emulators": 13}
    assert report["manifest_categories"] == {"doom": "Games"}


def test_validate_self_test_invalid_detects_expected_failures() -> None:
    result = run_cli("validate", "--self-test-invalid")
    report = parse_report(result.stdout)

    assert result.returncode == 0, result.stderr
    assert report["status"] == "ok"
    assert report["detected"] == [
        "duplicate-target",
        "alias-collision",
        "forbidden-source",
        "invalid-category",
        "invalid-path",
        "missing-source",
        "origin-sha256-mismatch",
        "unknown-cli-args",
        "missing-license",
        "sha256-mismatch",
    ]


def test_validate_rejects_unknown_cli_args() -> None:
    result = run_cli("validate", "--source", str(SOURCE_ROOT), "--bogus")

    assert result.returncode != 0
    assert "--bogus" in result.stderr


def test_pending_commands_reject_unknown_cli_args() -> None:
    result = run_cli("manifest", "--bogus")

    assert result.returncode != 0
    assert "--bogus" in result.stderr


def test_later_commands_report_not_implemented() -> None:
    for command in ("manifest", "build-all", "device-qa", "audit"):
        result = run_cli(command)
        report = parse_report(result.stdout)

        assert result.returncode == 2
        assert report["status"] == "not-implemented"
        assert report["command"] == command
        assert "Todo" in str(report["next_step"])


def test_vendor_gba_is_deterministic_and_rejects_wrong_ref(tmp_path: Path) -> None:
    destination = tmp_path / "vendor"
    first = run_cli("vendor", "--target", "gba", "--source", str(SOURCE_ROOT), "--source-ref", "2c6e1f22", "--destination", str(destination))
    second = run_cli("vendor", "--target", "gba", "--source", str(SOURCE_ROOT), "--source-ref", "2c6e1f22", "--destination", str(destination))
    wrong = run_cli("vendor", "--target", "gba", "--source", str(SOURCE_ROOT), "--source-ref", "0e80a99a", "--destination", str(destination))

    assert first.returncode == 0, first.stderr
    assert parse_report(first.stdout) == parse_report(second.stdout)
    assert wrong.returncode == 1
    assert parse_report(wrong.stdout)["issue_codes"] == ["source-ref-mismatch"]
    assert not list(destination.rglob("*.bin"))


def test_build_rejects_unknown_target_before_running_idf() -> None:
    result = run_cli("build", "--targets", "gb,not-real", "--append-manifest", "--size-limit", "0x700000")
    report = parse_report(result.stdout)

    assert result.returncode == 1
    assert report["status"] == "error"
    assert report["issue_codes"] == ["unknown-target"]
    assert report["target"] == "not-real"


def test_build_rejects_duplicate_target_before_running_idf() -> None:
    result = run_cli("build", "--targets", "gb,gb", "--append-manifest", "--size-limit", "0x700000")
    report = parse_report(result.stdout)

    assert result.returncode == 1
    assert report["status"] == "error"
    assert report["issue_codes"] == ["duplicate-target"]
    assert report["target"] == "gb"


def test_build_requires_existing_target_project() -> None:
    result = run_cli("build", "--targets", "nes", "--append-manifest", "--size-limit", "0x700000")
    report = parse_report(result.stdout)

    assert result.returncode == 1
    assert report["status"] == "error"
    assert report["issue_codes"] == ["missing-project"]
    assert report["target"] == "nes"


def test_finalize_artifact_rejects_missing_artifact(tmp_path: Path) -> None:
    from emulator_port.build import BuildOptions, finalize_artifact
    from emulator_port.inventory import TARGETS
    from emulator_port.models import AppName

    target = next(target for target in TARGETS if target.app_name == AppName("gb"))
    report = finalize_artifact(tmp_path, target, BuildOptions(tmp_path, ("gb",), True, 0x700000))

    assert report["status"] == "error"
    assert report["issue_codes"] == ["missing-artifact"]


def test_finalize_artifact_replaces_stale_manifest(tmp_path: Path) -> None:
    from emulator_port.build import BuildOptions, build_trailer, finalize_artifact
    from emulator_port.inventory import TARGETS
    from emulator_port.models import AppName

    target = next(target for target in TARGETS if target.app_name == AppName("gb"))
    artifact = tmp_path / "experiments" / "ota_apps" / "gb" / "build" / "gb.bin"
    artifact.parent.mkdir(parents=True)
    artifact.write_bytes(b"firmware" + build_trailer("Emulators", "old"))

    report = finalize_artifact(tmp_path, target, BuildOptions(tmp_path, ("gb",), True, 0x700000))

    assert report["target"] == "gb"
    assert report["manifest"] == {"category": "Emulators", "name": "gb", "crc": 2279197089}
    assert len(artifact.read_bytes()) == len(b"firmware") + 56


def test_finalize_artifact_enforces_post_manifest_size_limit(tmp_path: Path) -> None:
    from emulator_port.build import BuildOptions, finalize_artifact
    from emulator_port.inventory import TARGETS
    from emulator_port.models import AppName

    target = next(target for target in TARGETS if target.app_name == AppName("gb"))
    artifact = tmp_path / "experiments" / "ota_apps" / "gb" / "build" / "gb.bin"
    artifact.parent.mkdir(parents=True)
    artifact.write_bytes(b"firmware")

    report = finalize_artifact(tmp_path, target, BuildOptions(tmp_path, ("gb",), True, 8))

    assert report["status"] == "error"
    assert report["issue_codes"] == ["oversize"]


def test_finalize_artifact_reports_manifest_identity(tmp_path: Path) -> None:
    from emulator_port.build import BuildOptions, finalize_artifact
    from emulator_port.inventory import TARGETS
    from emulator_port.models import AppName

    target = next(target for target in TARGETS if target.app_name == AppName("gbc"))
    artifact = tmp_path / "experiments" / "ota_apps" / "gbc" / "build" / "gbc.bin"
    artifact.parent.mkdir(parents=True)
    artifact.write_bytes(b"firmware")

    report = finalize_artifact(tmp_path, target, BuildOptions(tmp_path, ("gbc",), True, 0x700000))

    assert report["target"] == "gbc"
    assert report["rom_root"] == "/roms/gbc"
    assert report["save_root"] == "/saves/gbc"
    assert report["manifest"] == {"category": "Emulators", "name": "gbc", "crc": 979460852}


def test_runtime_suite_rejects_unknown_case() -> None:
    result = run_cli("test", "--suite", "runtime", "--case", "not-real")
    report = parse_report(result.stdout)

    assert result.returncode == 1
    assert report["status"] == "error"
    assert report["issue_codes"] == ["unknown-case"]


def test_hardware_adapters_suite_uses_plan_hyphen_name() -> None:
    result = run_cli("test", "--suite", "hardware-adapters", "--case", "not-real")
    report = parse_report(result.stdout)

    assert result.returncode == 1
    assert report["status"] == "error"
    assert report["issue_codes"] == ["unknown-case"]
    assert report["known_cases"] == ["malformed-input", "signals", "template"]


def test_runtime_suite_lifecycle_runs_compiled_ctest() -> None:
    result = run_cli("test", "--suite", "runtime", "--case", "lifecycle", timeout=120)
    report = parse_report(result.stdout)

    assert result.returncode == 0, result.stdout + result.stderr
    assert report["status"] == "ok"
    assert report["case"] == "lifecycle"
    assert "runtime_lifecycle_tests" in str(report["stdout"])


def test_validate_rejects_tampered_vendor_digest(tmp_path: Path) -> None:
    vendor_root = tmp_path / "vendor" / "licenses"
    source_license = REPO_ROOT / "experiments" / "ota_apps" / "emulator_common" / "vendor" / "licenses" / "retro-go_COPYING"
    tampered_license = vendor_root / "retro-go_COPYING"
    vendor_root.mkdir(parents=True)
    shutil.copyfile(source_license, tampered_license)
    tampered_license.write_text("tampered\n", encoding="utf-8")

    result = run_cli("validate", "--source", str(SOURCE_ROOT), "--vendor-root", str(vendor_root))
    report = parse_report(result.stdout)

    assert result.returncode == 1
    assert report["status"] == "error"
    assert "sha256-mismatch" in report["issue_codes"]


def test_validate_rejects_missing_source(tmp_path: Path) -> None:
    missing_source = tmp_path / "missing-source"

    result = run_cli("validate", "--source", str(missing_source))
    report = parse_report(result.stdout)

    assert result.returncode == 1
    assert report["status"] == "error"
    assert report["issue_codes"] == ["missing-source"]


def test_validate_rejects_nondirectory_source(tmp_path: Path) -> None:
    file_source = tmp_path / "source-file"
    file_source.write_text("not a directory\n", encoding="utf-8")

    result = run_cli("validate", "--source", str(file_source))
    report = parse_report(result.stdout)

    assert result.returncode == 1
    assert report["status"] == "error"
    assert report["issue_codes"] == ["invalid-source"]


def test_validate_checks_target_source_roots() -> None:
    from emulator_port.inventory import LICENSES, TARGETS
    from emulator_port.models import SourcePath
    from emulator_port.validation import validate_inventory

    bad_target = replace(TARGETS[0], source_roots=(SourcePath("retro-core/components/not-real"),))
    report = validate_inventory(
        SOURCE_ROOT,
        REPO_ROOT / "experiments" / "ota_apps" / "emulator_common" / "vendor" / "licenses",
        (bad_target, *TARGETS[1:]),
        LICENSES,
    )

    assert "missing-source-root" in [issue.code for issue in report.issues]


def test_validate_checks_gba_historical_source_root() -> None:
    from emulator_port.inventory import LICENSES, TARGETS
    from emulator_port.models import AppName, SourcePath
    from emulator_port.validation import validate_inventory

    targets = tuple(
        replace(target, source_roots=(SourcePath("gbsp-not-real"),)) if target.app_name == AppName("gba") else target
        for target in TARGETS
    )
    report = validate_inventory(
        SOURCE_ROOT,
        REPO_ROOT / "experiments" / "ota_apps" / "emulator_common" / "vendor" / "licenses",
        targets,
        LICENSES,
    )

    assert "missing-source-root" in [issue.code for issue in report.issues]


def test_validate_finishes_under_timeout() -> None:
    result = run_cli("validate", "--source", str(SOURCE_ROOT), timeout=10)

    assert result.returncode == 0, result.stderr
