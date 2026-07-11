from __future__ import annotations

import argparse
import json
import sys
from collections.abc import Mapping
from pathlib import Path
from typing import Final

from .build import BuildOptions, run_build
from .models import JsonValue, ValidationIssue, ValidationReport
from .suites import SuiteRunOptions, run_suite
from .validation import report_json, self_test_invalid, validate_inventory
from .vendor import VendorOptions, vendor_gba


DEFAULT_SOURCE: Final = Path("/home/netwan/myprojects/esp32-retro-go-pixel")
DEFAULT_VENDOR_ROOT: Final = Path("experiments/ota_apps/emulator_common/vendor/licenses")
PENDING_COMMANDS: Final[dict[str, str]] = {
    "manifest": "Todo 15 wires manifest append and size gates.",
    "build-all": "Todo 15 owns all-target release builds.",
    "device-qa": "Todo 5-15 require device fixtures and app binaries.",
    "audit": "Todo 15 and the final verification wave own full audits.",
}


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="emulator_port.py")
    subparsers = parser.add_subparsers(dest="command", required=True)
    validate = subparsers.add_parser("validate")
    validate.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    validate.add_argument("--vendor-root", type=Path, default=DEFAULT_VENDOR_ROOT)
    validate.add_argument("--self-test-invalid", action="store_true")
    test = subparsers.add_parser("test")
    test.add_argument("--suite")
    test.add_argument("--target")
    test.add_argument("--cases")
    test.add_argument("--case")
    test.add_argument("--sanitizers", action="store_true")
    build = subparsers.add_parser("build")
    build.add_argument("--targets", required=True)
    build.add_argument("--append-manifest", action="store_true")
    build.add_argument("--size-limit", required=True, type=parse_size_limit)
    vendor = subparsers.add_parser("vendor")
    vendor.add_argument("--target", required=True)
    vendor.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    vendor.add_argument("--source-ref", required=True)
    vendor.add_argument("--destination", type=Path, default=Path("experiments/ota_apps/emulator_common/vendor"))
    for command in PENDING_COMMANDS:
        subparsers.add_parser(command)
    return parser


def print_json(value: Mapping[str, JsonValue]) -> None:
    json.dump(value, sys.stdout, sort_keys=True)
    sys.stdout.write("\n")


def run_validate(args: argparse.Namespace) -> int:
    vendor_root = args.vendor_root if args.vendor_root.is_absolute() else Path.cwd() / args.vendor_root
    if args.self_test_invalid:
        report = self_test_invalid(args.source, vendor_root)
        print_json(report)
        return 0 if report["status"] == "ok" else 1
    source_issue = source_boundary_issue(args.source)
    if source_issue is not None:
        print_json(report_json(ValidationReport(issues=(source_issue,), targets=())))
        return 1
    report = validate_inventory(args.source, vendor_root)
    print_json(report_json(report))
    return 0 if report.passed else 1


def source_boundary_issue(source: Path) -> ValidationIssue | None:
    if not source.exists():
        return ValidationIssue("missing-source", f"source root does not exist: {source}")
    if not source.is_dir():
        return ValidationIssue("invalid-source", f"source root is not a directory: {source}")
    return None


def run_pending(command: str) -> int:
    print_json({"status": "not-implemented", "command": command, "next_step": PENDING_COMMANDS[command]})
    return 2


def run_test(args: argparse.Namespace) -> int:
    suite = args.suite or args.target
    if suite is None:
        print_json({"status": "error", "issue_codes": ["missing-suite-or-target"]})
        return 1
    case = args.case
    if args.cases is not None:
        requested = parse_targets(args.cases)
        if suite == "gba" and set(requested) == {"save-types", "allocation"}:
            case = None
        else:
            print_json({"status": "error", "issue_codes": ["unknown-cases"], "cases": list(requested)})
            return 1
    try:
        result = run_suite(SuiteRunOptions(repo_root=Path.cwd(), suite=suite, case=case, sanitizers=args.sanitizers))
    except KeyboardInterrupt:
        print_json({"status": "error", "issue_codes": ["interrupted"], "suite": args.suite, "case": args.case or "all"})
        return 130
    print_json(result.report)
    return result.exit_code


def parse_size_limit(value: str) -> int:
    return int(value, 0)


def parse_targets(value: str) -> tuple[str, ...]:
    return tuple(part.strip() for part in value.split(",") if part.strip())


def run_build_command(args: argparse.Namespace) -> int:
    exit_code, report = run_build(BuildOptions(repo_root=Path.cwd(), target_names=parse_targets(args.targets), append_manifest=args.append_manifest, size_limit=args.size_limit))
    print_json(report)
    return exit_code


def run_vendor_command(args: argparse.Namespace) -> int:
    if args.target != "gba":
        print_json({"status": "error", "issue_codes": ["unsupported-vendor-target"], "target": args.target})
        return 1
    exit_code, report = vendor_gba(VendorOptions(args.source, args.source_ref, args.destination))
    print_json(report)
    return exit_code


def main(argv: list[str] | None = None) -> int:
    args = make_parser().parse_args(argv)
    if args.command == "validate":
        return run_validate(args)
    if args.command == "test":
        return run_test(args)
    if args.command == "build":
        return run_build_command(args)
    if args.command == "vendor":
        return run_vendor_command(args)
    return run_pending(args.command)
