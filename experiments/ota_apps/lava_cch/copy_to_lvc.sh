#!/usr/bin/env bash

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEST_DIR="/media/netwan/RETROFW/LavaXOS/PROGRAM/Lava/中国象棋.app"
DEST_LAV="$DEST_DIR/中国象棋.lav"
SRC_LAV="$SCRIPT_DIR/main.lav"

if [ ! -f "$SRC_LAV" ]; then
    echo "Error: main.lav not found in $SCRIPT_DIR" >&2
    exit 1
fi

if [ ! -d "$SCRIPT_DIR/LavaData" ]; then
    echo "Error: LavaData directory not found in $SCRIPT_DIR" >&2
    exit 1
fi

if [ ! -f "$SCRIPT_DIR/icon.bmp" ]; then
    echo "Error: icon.bmp not found in $SCRIPT_DIR" >&2
    exit 1
fi

mkdir -p "$DEST_DIR"
mkdir -p "$DEST_DIR/LavaData"

cp -f "$SRC_LAV" "$DEST_LAV"
cp -a "$SCRIPT_DIR/LavaData/." "$DEST_DIR/LavaData/"
cp -f "$SCRIPT_DIR/icon.bmp" "$DEST_DIR/icon.bmp"

echo "Installed:"
echo "  $SRC_LAV -> $DEST_LAV"
echo "  $SCRIPT_DIR/LavaData -> $DEST_DIR/LavaData"
echo "  $SCRIPT_DIR/icon.bmp -> $DEST_DIR/icon.bmp"
