from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
from typing import Any


SCRIPT = Path(__file__).parents[1] / "tools" / "serial_sd_client.py"
SPEC = importlib.util.spec_from_file_location("serial_sd_client", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
serial_sd_client = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = serial_sd_client
SPEC.loader.exec_module(serial_sd_client)


class FakeSerial:
    def __init__(self) -> None:
        self.writes: list[bytes] = []

    def write(self, data: bytes) -> None:
        self.writes.append(data)

    def flush(self) -> None:
        pass


class FakeClock:
    def __init__(self) -> None:
        self.now = 0.0

    def monotonic(self) -> float:
        return self.now


def make_client(timeout_seconds: float) -> tuple[Any, FakeSerial]:
    client = object.__new__(serial_sd_client.SerialServiceClient)
    fake_serial = FakeSerial()
    client._serial = fake_serial
    client._timeout_seconds = timeout_seconds
    return client, fake_serial


def test_enter_retries_until_launcher_control_is_ready(monkeypatch) -> None:
    client, fake_serial = make_client(5.0)
    clock = FakeClock()
    responses = iter(["", "", "", "SFS1 ENTERING", "", "SFS1 READY"])

    def read_line() -> str:
        clock.now += 0.4
        return next(responses)

    client.read_line = read_line
    monkeypatch.setattr(serial_sd_client.time, "monotonic", clock.monotonic)

    assert client.enter_service() == "SFS1 READY"
    assert fake_serial.writes == [b"SFS1 ENTER\n", b"SFS1 ENTER\n"]


def test_enter_stops_retrying_after_entering_response(monkeypatch) -> None:
    client, fake_serial = make_client(5.0)
    clock = FakeClock()
    responses = iter(["SFS1 ENTERING", "", "", "", "SFS1 READY"])

    def read_line() -> str:
        clock.now += 0.6
        return next(responses)

    client.read_line = read_line
    monkeypatch.setattr(serial_sd_client.time, "monotonic", clock.monotonic)

    assert client.enter_service() == "SFS1 READY"
    assert fake_serial.writes == [b"SFS1 ENTER\n"]


def test_enter_honors_configured_timeout(monkeypatch) -> None:
    client, fake_serial = make_client(1.5)
    clock = FakeClock()

    def read_line() -> str:
        clock.now += 0.6
        return ""

    client.read_line = read_line
    monkeypatch.setattr(serial_sd_client.time, "monotonic", clock.monotonic)

    assert client.enter_service() == "ERR timeout"
    assert fake_serial.writes == [b"SFS1 ENTER\n", b"SFS1 ENTER\n"]
