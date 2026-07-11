from __future__ import annotations

import hashlib
import subprocess
import sys
from collections import Counter
from dataclasses import replace
from pathlib import Path, PurePosixPath
from typing import Final

from .inventory import BASE, LICENSES, TARGETS
from .models import AppName, JsonValue, LicenseId, LicenseRecord, ManifestCategory, Sha256, SourcePath, Target, ValidationIssue, ValidationReport


EXPECTED_TARGET_COUNT: Final = 14
FORBIDDEN_SOURCE_PARTS: Final = frozenset({"launcher", "network", "updater", "covers", "screenshots"})


def sha256_bytes(data: bytes) -> Sha256:
    return Sha256(hashlib.sha256(data).hexdigest())


def sha256_file(path: Path) -> Sha256:
    return sha256_bytes(path.read_bytes())


GBA_SOURCE_REF: Final = "2c6e1f22"


def git_blob(source_root: Path, source_ref: str, origin_path: str) -> bytes:
    result = subprocess.run(
        ["git", "-C", str(source_root), "show", f"{source_ref}:{origin_path}"],
        capture_output=True,
        timeout=15,
        check=False,
    )
    if result.returncode != 0:
        return b""
    return result.stdout


def git_tree_exists(source_root: Path, source_ref: str, origin_path: str) -> bool:
    result = subprocess.run(
        ["git", "-C", str(source_root), "ls-tree", source_ref, origin_path],
        capture_output=True,
        timeout=15,
        check=False,
    )
    return result.returncode == 0 and len(result.stdout.strip()) > 0


def license_origin_bytes(source_root: Path, record: LicenseRecord) -> bytes:
    if record.source_ref is None:
        return (source_root / str(record.origin_path)).read_bytes()
    return git_blob(source_root, record.source_ref, str(record.origin_path))


def validate_inventory(source_root: Path, vendor_root: Path, targets: tuple[Target, ...] = TARGETS, licenses: tuple[LicenseRecord, ...] = LICENSES) -> ValidationReport:
    issues = [*validate_targets(source_root, targets, licenses), *validate_licenses(source_root, vendor_root, licenses)]
    return ValidationReport(issues=tuple(issues), targets=targets)


def validate_targets(source_root: Path, targets: tuple[Target, ...], licenses: tuple[LicenseRecord, ...]) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    license_ids = {record.license_id for record in licenses}
    names = [target.app_name for target in targets]
    if len(targets) != EXPECTED_TARGET_COUNT:
        issues.append(ValidationIssue("target-count", f"expected {EXPECTED_TARGET_COUNT}, got {len(targets)}"))
    for name, count in Counter(names).items():
        if count > 1:
            issues.append(ValidationIssue("duplicate-target", f"{name} appears {count} times"))
    aliases = [alias for target in targets for alias in target.aliases]
    for alias, count in Counter(aliases).items():
        if count > 1 or alias in names:
            issues.append(ValidationIssue("alias-collision", f"alias {alias} is not unique"))
    for target in targets:
        issues.extend(validate_target(source_root, target, license_ids))
    return issues


def validate_target(upstream_root: Path, target: Target, license_ids: set[LicenseId]) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    expected_category = ManifestCategory.GAMES if target.app_name == AppName("doom") else ManifestCategory.EMULATORS
    if target.manifest_category != expected_category:
        issues.append(ValidationIssue("invalid-category", f"{target.app_name} must use {expected_category.value}"))
    for path in (target.rom_root, target.save_root):
        if not is_safe_sd_root(path, str(target.app_name)):
            issues.append(ValidationIssue("invalid-path", f"{target.app_name} has unsafe path {path}"))
    if "zip" in target.rom_extensions:
        issues.append(ValidationIssue("archive-extension", f"{target.app_name} includes forbidden zip support"))
    for target_source_root in target.source_roots:
        parts = PurePosixPath(str(target_source_root)).parts
        if FORBIDDEN_SOURCE_PARTS.intersection(parts):
            issues.append(ValidationIssue("forbidden-source", f"{target.app_name} uses {target_source_root}"))
        if not source_root_exists(upstream_root, target, str(target_source_root)):
            issues.append(ValidationIssue("missing-source-root", f"{target.app_name} source root missing: {target_source_root}"))
    for license_id in target.license_ids:
        if license_id not in license_ids:
            issues.append(ValidationIssue("missing-license", f"{target.app_name} references {license_id}"))
    return issues


def source_root_exists(upstream_root: Path, target: Target, source_path: str) -> bool:
    if target.app_name == AppName("gba"):
        return git_tree_exists(upstream_root, GBA_SOURCE_REF, source_path)
    return (upstream_root / source_path).exists()


def is_safe_sd_root(path: PurePosixPath, app_name: str) -> bool:
    return path.is_absolute() and ".." not in path.parts and path.name == app_name and len(path.parts) == 3


def validate_licenses(source_root: Path, vendor_root: Path, licenses: tuple[LicenseRecord, ...]) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    for record in licenses:
        vendor_path = vendor_root / record.vendor_name
        if not vendor_path.exists():
            issues.append(ValidationIssue("missing-license", f"missing {vendor_path}"))
            continue
        if sha256_file(vendor_path) != record.sha256:
            issues.append(ValidationIssue("sha256-mismatch", f"{record.vendor_name} does not match vendored digest"))
        origin_digest = sha256_bytes(license_origin_bytes(source_root, record))
        if origin_digest != record.origin_sha256:
            issues.append(ValidationIssue("origin-sha256-mismatch", f"{record.origin_path} does not match origin digest"))
    return issues


def report_json(report: ValidationReport) -> dict[str, JsonValue]:
    category_counter = Counter(target.manifest_category.value for target in report.targets)
    games = [str(target.app_name) for target in report.targets if target.manifest_category is ManifestCategory.GAMES]
    return {
        "status": "ok" if report.passed else "error",
        "target_count": len(report.targets),
        "issue_codes": [issue.code for issue in report.issues],
        "issues": [{"code": issue.code, "detail": issue.detail} for issue in report.issues],
        "categories": {"Games/doom": len(games), "Emulators": category_counter[ManifestCategory.EMULATORS.value]},
        "manifest_categories": {name: "Games" for name in games},
    }


def self_test_invalid(source_root: Path, vendor_root: Path) -> dict[str, JsonValue]:
    duplicate = TARGETS + (TARGETS[0],)
    alias_collision = (replace(TARGETS[0], aliases=(str(TARGETS[1].app_name),)), *TARGETS[1:])
    forbidden_source = (replace(TARGETS[0], source_roots=(SourcePath("launcher/main"),)), *TARGETS[1:])
    invalid_category = (TARGETS[0], replace(TARGETS[-1], manifest_category=ManifestCategory.EMULATORS))
    invalid_path = (replace(TARGETS[0], rom_root=PurePosixPath("/roms/../nes")),)
    missing_license = tuple(record for record in LICENSES if record.license_id != BASE)
    bad_hash = (replace(LICENSES[0], sha256=Sha256("0" * 64)), *LICENSES[1:])
    bad_origin_hash = (replace(LICENSES[0], origin_sha256=Sha256("0" * 64)), *LICENSES[1:])
    cases = {
        "duplicate-target": validate_inventory(source_root, vendor_root, duplicate).issues,
        "alias-collision": validate_inventory(source_root, vendor_root, alias_collision).issues,
        "forbidden-source": validate_inventory(source_root, vendor_root, forbidden_source).issues,
        "invalid-category": validate_inventory(source_root, vendor_root, invalid_category + TARGETS[1:-1]).issues,
        "invalid-path": validate_inventory(source_root, vendor_root, invalid_path + TARGETS[1:]).issues,
        "missing-license": validate_inventory(source_root, vendor_root, TARGETS, missing_license).issues,
        "origin-sha256-mismatch": validate_inventory(source_root, vendor_root, TARGETS, bad_origin_hash).issues,
        "sha256-mismatch": validate_inventory(source_root, vendor_root, TARGETS, bad_hash).issues,
    }
    cli_cases = {"missing-source": cli_case_failed("validate", "--source", "/tmp/miaos-emulator-port-missing-source"), "unknown-cli-args": cli_case_failed("validate", "--bogus")}
    expected = ("duplicate-target", "alias-collision", "forbidden-source", "invalid-category", "invalid-path", "missing-source", "origin-sha256-mismatch", "unknown-cli-args", "missing-license", "sha256-mismatch")
    detected = [code for code in expected if case_detected(code, cases, cli_cases)]
    return {"status": "ok" if len(detected) == len(expected) else "error", "detected": detected}


def cli_case_failed(*args: str) -> bool:
    script = Path(__file__).resolve().parents[1] / "emulator_port.py"
    result = subprocess.run(
        [sys.executable, str(script), *args],
        capture_output=True,
        timeout=15,
        check=False,
    )
    return result.returncode != 0


def case_detected(code: str, cases: dict[str, tuple[ValidationIssue, ...]], cli_cases: dict[str, bool]) -> bool:
    if code in cli_cases:
        return cli_cases[code]
    return any(issue.code == code for issue in cases[code])
