#!/usr/bin/env python3
"""
Append OtaAppManifest trailer to an ESP-IDF firmware binary.

Usage:
    python tools/append_manifest.py \\
        --input build/calculator.bin \\
        --category Utils --name calculator

The script appends a 72-byte MIA2 trailer containing a sortable build time and
the size/CRC32 of the firmware bytes before the trailer.

The ESP-IDF bootloader reads exactly image_len bytes and ignores the
trailer, so it is safe for direct esptool flashing and OTA updates.
"""

import argparse
import os
import struct
import time
import zlib


MIA1_MAGIC = 0x3141494D
MIA2_MAGIC = 0x3241494D
MIA1_SIZE = 56
MIA2_SIZE = 72


def build_trailer(category: str, name: str, image: bytes, build_epoch: int) -> bytes:
    cat_bytes = category.encode('utf-8').ljust(16, b'\0')[:16]
    nam_bytes = name.encode('utf-8').ljust(32, b'\0')[:32]
    payload = struct.pack('<I16s32sQII', MIA2_MAGIC, cat_bytes, nam_bytes,
                          build_epoch, len(image), zlib.crc32(image) & 0xFFFFFFFF)
    return payload + struct.pack('<I', zlib.crc32(payload) & 0xFFFFFFFF)


def parse_trailer(data: bytes):
    """Extract manifest from the trailer if valid; return dict or None."""
    for size, magic in ((MIA2_SIZE, MIA2_MAGIC), (MIA1_SIZE, MIA1_MAGIC)):
        if len(data) < size:
            continue
        trailer = data[-size:]
        actual_magic, = struct.unpack_from('<I', trailer)
        stored_crc, = struct.unpack_from('<I', trailer, size - 4)
        if actual_magic != magic or zlib.crc32(trailer[:-4]) & 0xFFFFFFFF != stored_crc:
            continue
        result = {
            'magic': magic,
            'size': size,
            'category': trailer[4:20].rstrip(b'\0').decode('utf-8', errors='replace'),
            'name': trailer[20:52].rstrip(b'\0').decode('utf-8', errors='replace'),
        }
        if magic == MIA2_MAGIC:
            result['build_epoch'], result['image_size'], result['image_crc'] = struct.unpack_from('<QII', trailer, 52)
        return result
    return None


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
    parser.add_argument('--build-epoch', type=int,
                        help='Sortable build timestamp (default: SOURCE_DATE_EPOCH or current time)')
    args = parser.parse_args()

    output = args.output or args.input

    with open(args.input, 'rb') as f:
        data = f.read()

    existing = parse_trailer(data)
    if existing:
        print(f'NOTE: input already has manifest: '
              f'category={existing["category"]}, name={existing["name"]}')
        data = data[:-existing['size']]

    build_epoch = args.build_epoch
    if build_epoch is None:
        build_epoch = int(os.environ.get('SOURCE_DATE_EPOCH', time.time()))
    trailer = build_trailer(args.category, args.name, data, build_epoch)

    with open(output, 'wb') as f:
        f.write(data)
        f.write(trailer)

    print(f'Appended MIA2 manifest: category={args.category}, name={args.name}, build={build_epoch}')
    print(f'Output: {output}')
    print(f'Size: {len(data)} -> {len(data) + len(trailer)} bytes')


if __name__ == '__main__':
    main()
