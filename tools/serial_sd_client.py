#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.13"
# dependencies = [
#   "pyserial",
#   "typer",
#   "rich",
# ]
# ///

# ─── How to run ───
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


app = typer.Typer(no_args_is_help=True)
UPLOAD_CHUNK_SIZE = 1024
UPLOAD_CHUNK_DELAY_SECONDS = 0.01


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

    def read_line(self) -> str:
        line = self._serial.readline().decode("utf-8", errors="replace").strip()
        return line

    def read_protocol_line(self) -> str:
        deadline = time.time() + 5.0
        while time.time() < deadline:
            line = self.read_line()
            if not line:
                continue
            if line.startswith(("OK", "ERR", "READY", "ITEM", "END")):
                return line
        return ""

    def put(self, local_path: Path, remote_path: str) -> str:
        size = local_path.stat().st_size
        response = self.send_command(f"PUT {remote_path} {size}")
        if response != "READY":
            return response
        with local_path.open("rb") as file:
          while chunk := file.read(UPLOAD_CHUNK_SIZE):
              self._serial.write(chunk)
              self._serial.flush()
              time.sleep(UPLOAD_CHUNK_DELAY_SECONDS)
        return self.read_protocol_line()


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


if __name__ == "__main__":
    app()
