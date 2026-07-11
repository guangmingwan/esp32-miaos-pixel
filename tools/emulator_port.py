#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.13"
# dependencies = []
# ///

# ─── How to run ───
# 1. Install uv (if not installed):
#      curl -LsSf https://astral.sh/uv/install.sh | sh
# 2. Run directly (no venv, no pip install needed):
#      uv run tools/emulator_port.py validate --source /path/to/esp32-retro-go-pixel
# 3. Or make executable and run:
#      chmod +x tools/emulator_port.py && ./tools/emulator_port.py validate --source /path/to/esp32-retro-go-pixel
# ──────────────────

from __future__ import annotations

import sys

from emulator_port.cli import main


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
