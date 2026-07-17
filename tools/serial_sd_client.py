#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.13"
# dependencies = [
#   "pyserial",
#   "typer",
#   "rich",
# ]
# ///

# ─── USB VCP usage ───
# 1. Install uv (if not installed):
#      curl -LsSf https://astral.sh/uv/install.sh | sh
# 2. Run directly (no venv, no pip install needed):
#      uv run tools/serial_sd_client.py --help
# 3. Or make executable and run:
#      chmod +x tools/serial_sd_client.py && ./tools/serial_sd_client.py --help
# ──────────────────

from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path
import subprocess
import time

import serial
import typer
from rich import print as rprint


app = typer.Typer(
    no_args_is_help=True,
    help="Transfer files over the ESP32-S3 USB CDC/JTAG virtual COM port (VCP).",
)
VCP_WINDOW_SIZE = 6144
UPLOAD_CHUNK_SIZE = VCP_WINDOW_SIZE
DOWNLOAD_CHUNK_SIZE = 4096


@dataclass(frozen=True, slots=True)
class ClientConfig:
    port: str
    baud: int
    timeout_seconds: float


class SerialServiceClient:
    def __init__(self, config: ClientConfig) -> None:
        self._prepare_port(config.port)
        self._serial = serial.Serial(config.port, config.baud, timeout=config.timeout_seconds)
        time.sleep(0.2)
        self._serial.reset_input_buffer()
        self._drain_startup_noise()

    def _prepare_port(self, port: str) -> None:
        if os.name != "posix":
            return
        subprocess.run(["stty", "-F", port, "-hupcl"], check=False, capture_output=True)

    def _drain_startup_noise(self) -> None:
        deadline = time.time() + 0.5
        while time.time() < deadline:
            line = self._serial.readline()
            if not line:
                break

    def close(self) -> None:
        self._serial.close()

    def send_command(self, command: str) -> str:
        self._serial.write((command + "\n").encode("utf-8"))
        self._serial.flush()
        return self.read_protocol_line()

    def enter_service(self) -> str:
        self._serial.write(b"SFS1 ENTER\n")
        self._serial.flush()
        return self._wait_for_line("SFS1 READY")

    def exit_service(self) -> str:
        self._serial.write(b"SFS1 EXIT\n")
        self._serial.flush()
        return self._wait_for_line("SFS1 EXITING")

    def _wait_for_line(self, expected: str) -> str:
        deadline = time.time() + 5.0
        while time.time() < deadline:
            line = self.read_line()
            if line == expected:
                return line
        return "ERR timeout"

    def read_line(self) -> str:
        line = self._serial.readline().decode("utf-8", errors="replace").strip()
        return line

    def read_protocol_line(self) -> str:
        deadline = time.time() + 5.0
        while time.time() < deadline:
            line = self.read_line()
            if not line:
                continue
            if line.startswith(("OK", "ERR", "READY", "ACK", "DATA", "ITEM", "END")):
                return line
        return ""

    def put(self, local_path: Path, remote_path: str) -> str:
        size = local_path.stat().st_size
        response = self.send_command(f"PUT {remote_path} {size}")
        if response != "READY":
            return response
        sent = 0
        with local_path.open("rb") as file:
            while chunk := file.read(UPLOAD_CHUNK_SIZE):
                self._serial.write(chunk)
                self._serial.flush()
                sent += len(chunk)
                response = self.read_protocol_line()
                if sent == size:
                    return response or "ERR missing upload completion"
                if response != f"ACK {sent}":
                    return response or "ERR missing upload ack"
        return self.read_protocol_line()

    def get(self, remote_path: str, local_path: Path) -> str:
        response = self.send_command(f"GET {remote_path}")
        if not response.startswith("DATA "):
            return response
        try:
            expected_size = int(response.removeprefix("DATA "))
        except ValueError:
            return "ERR invalid size"

        partial_path = local_path.with_name(local_path.name + ".part")
        received = 0
        try:
            with partial_path.open("wb") as file:
                while received < expected_size:
                    chunk = self._serial.read(min(DOWNLOAD_CHUNK_SIZE, expected_size - received))
                    if not chunk:
                        return "ERR download timeout"
                    file.write(chunk)
                    received += len(chunk)
            response = self.read_protocol_line()
            if response != "OK sent":
                return response or "ERR missing completion"
            partial_path.replace(local_path)
            return response
        finally:
            if partial_path.exists():
                partial_path.unlink()


def with_client(port: str, baud: int, timeout_seconds: float) -> SerialServiceClient:
    return SerialServiceClient(ClientConfig(port=port, baud=baud, timeout_seconds=timeout_seconds))


@app.command()
def ping(port: str = "/dev/ttyACM0", baud: int = 115200, timeout_seconds: float = 2.0) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.send_command("PING"))
    finally:
        client.close()


@app.command()
def info(port: str = "/dev/ttyACM0", baud: int = 115200, timeout_seconds: float = 2.0) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.send_command("INFO"))
    finally:
        client.close()


@app.command("enter")
def enter_service(
    port: str = "/dev/ttyACM0",
    baud: int = 115200,
    timeout_seconds: float = 2.0,
) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.enter_service())
    finally:
        client.close()


@app.command("exit")
def exit_service(
    port: str = "/dev/ttyACM0",
    baud: int = 115200,
    timeout_seconds: float = 2.0,
) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.exit_service())
    finally:
        client.close()


@app.command()
def list_dir(
    remote_path: str = "/",
    port: str = "/dev/ttyACM0",
    baud: int = 115200,
    timeout_seconds: float = 2.0,
) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.send_command(f"LIST {remote_path}"))
        while True:
            line = client.read_line()
            if line == "END" or line == "":
                break
            rprint(line)
    finally:
        client.close()


@app.command()
def mkdir(
    remote_path: str,
    port: str = "/dev/ttyACM0",
    baud: int = 115200,
    timeout_seconds: float = 2.0,
) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.send_command(f"MKDIR {remote_path}"))
    finally:
        client.close()


@app.command()
def delete(
    remote_path: str,
    port: str = "/dev/ttyACM0",
    baud: int = 115200,
    timeout_seconds: float = 2.0,
) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.send_command(f"DELETE {remote_path}"))
    finally:
        client.close()


@app.command()
def rename(
    old_remote_path: str,
    new_remote_path: str,
    port: str = "/dev/ttyACM0",
    baud: int = 115200,
    timeout_seconds: float = 2.0,
) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.send_command(f"RENAME {old_remote_path}\t{new_remote_path}"))
    finally:
        client.close()


@app.command()
def put(
    local_path: Path,
    remote_path: str,
    port: str = "/dev/ttyACM0",
    baud: int = 115200,
    timeout_seconds: float = 5.0,
) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.put(local_path, remote_path))
    finally:
        client.close()


@app.command()
def get(
    remote_path: str,
    local_path: Path,
    port: str = "/dev/ttyACM0",
    baud: int = 115200,
    timeout_seconds: float = 5.0,
) -> None:
    client = with_client(port, baud, timeout_seconds)
    try:
        rprint(client.get(remote_path, local_path))
    finally:
        client.close()


if __name__ == "__main__":
    app()
