#!/usr/bin/env python3
"""
Append OtaAppManifest trailer to an ESP-IDF firmware binary.

Usage:
    python tools/append_manifest.py \\
        --input build/calculator.bin \\
        --category Utils --name calculator

The script appends a 56-byte trailer:
    magic(4) + category(16) + name(32) + crc32(4)

The ESP-IDF bootloader reads exactly image_len bytes and ignores the
trailer, so it is safe for direct esptool flashing and OTA updates.
"""

import argparse
import struct
import zlib


def compute_crc(magic: int, category: bytes, name: bytes) -> int:
    crc_data = struct.pack('<I', magic) + category + name
    return zlib.crc32(crc_data) & 0xFFFFFFFF


def build_trailer(category: str, name: str) -> bytes:
    magic = 0x3141494D  # MIA_APP_MANIFEST_MAGIC = "MIA1"
    cat_bytes = category.encode('utf-8').ljust(16, b'\0')[:16]
    nam_bytes = name.encode('utf-8').ljust(32, b'\0')[:32]
    crc = compute_crc(magic, cat_bytes, nam_bytes)
    return struct.pack('<I', magic) + cat_bytes + nam_bytes + struct.pack('<I', crc)


def parse_trailer(data: bytes):
    """Extract manifest from the trailer if valid; return dict or None."""
    if len(data) < MIA_MANIFEST_TRAILER_SIZE:
        return None
    trailer = data[-MIA_MANIFEST_TRAILER_SIZE:]
    magic, = struct.unpack_from('<I', trailer, 0)
    if magic != 0x3141494D:
        return None
    cat_bytes = trailer[4:20]
    nam_bytes = trailer[20:52]
    stored_crc, = struct.unpack_from('<I', trailer, 52)

    expected_crc = compute_crc(magic, cat_bytes, nam_bytes)
    if stored_crc != expected_crc:
        return None

    category = cat_bytes.rstrip(b'\0').decode('utf-8', errors='replace')
    name = nam_bytes.rstrip(b'\0').decode('utf-8', errors='replace')
    return {'category': category, 'name': name, 'crc': stored_crc}


MIA_MANIFEST_TRAILER_SIZE = 56


def main():
    parser = argparse.ArgumentParser(
        description='Append OTA app manifest trailer to a firmware binary.')
    parser.add_argument('--input', required=True,
                        help='Input firmware binary (.bin)')
    parser.add_argument('--output',
                        help='Output path (default: overwrite input)')
    parser.add_argument('--category', required=True,
                        help='App category, e.g. Utils, Games, Settings')
    parser.add_argument('--name', required=True,
                        help='App name, e.g. calculator, minesweeper')
    args = parser.parse_args()

    output = args.output or args.input

    with open(args.input, 'rb') as f:
        data = f.read()

    trailer = build_trailer(args.category, args.name)

    # Check if already has a manifest
    existing = parse_trailer(data)
    if existing:
        print(f'NOTE: input already has manifest: '
              f'category={existing["category"]}, name={existing["name"]}')
        if existing['category'] == args.category and existing['name'] == args.name:
            print('Manifest unchanged, nothing to do.')
            return

    with open(output, 'wb') as f:
        f.write(data)
        f.write(trailer)

    print(f'Appended manifest: category={args.category}, name={args.name}')
    print(f'Output: {output}')
    print(f'Size: {len(data)} -> {len(data) + len(trailer)} bytes')


if __name__ == '__main__':
    main()
