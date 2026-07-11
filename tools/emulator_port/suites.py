from __future__ import annotations

import importlib
from dataclasses import dataclass
from pathlib import Path
from types import ModuleType
from typing import Protocol

from .models import JsonValue


@dataclass(frozen=True, slots=True)
class SuiteRunOptions:
    repo_root: Path
    suite: str
    case: str | None
    sanitizers: bool


@dataclass(frozen=True, slots=True)
class SuiteRunResult:
    exit_code: int
    report: dict[str, JsonValue]


class SuiteRunner(Protocol):
    def __call__(self, options: SuiteRunOptions) -> SuiteRunResult: ...


def valid_suite_id(suite: str) -> bool:
    return suite != "" and all(char.isalnum() or char in {"_", "-"} for char in suite)


def module_stem(suite: str) -> str:
    return suite.replace("-", "_")


def canonical_suite_name(stem: str) -> str:
    return stem.replace("_", "-")


def known_suites() -> list[str]:
    suite_dir = Path(__file__).resolve().parent
    names = [path.name.removesuffix("_suite.py") for path in suite_dir.glob("*_suite.py")]
    return sorted(canonical_suite_name(name) for name in names if valid_suite_id(name))


def load_suite_module(suite: str) -> ModuleType | None:
    if not valid_suite_id(suite):
        return None
    stem = module_stem(suite)
    try:
        return importlib.import_module(f".{stem}_suite", __package__)
    except ModuleNotFoundError as error:
        if error.name == f"{__package__}.{stem}_suite":
            return None
        raise


def load_runner(suite: str) -> SuiteRunner | None:
    module = load_suite_module(suite)
    if module is None:
        return None
    runner = getattr(module, f"run_{module_stem(suite)}_suite", None)
    if not callable(runner):
        return None
    return runner


def run_suite(options: SuiteRunOptions) -> SuiteRunResult:
    runner = load_runner(options.suite)
    if runner is None:
        return SuiteRunResult(
            1,
            {
                "status": "error",
                "issue_codes": ["unknown-suite"],
                "suite": options.suite,
                "known_suites": known_suites(),
            },
        )
    return runner(options)
