#!/usr/bin/env python3
"""Check raythrower's PNG and Radiance HDR output against its raw image.

    check_image.py <image.raw> --png <file.png>
    check_image.py <image.raw> --hdr <file.hdr> [--clip]

The PNG must decode (with Python's own zlib) to exactly the raw image's
pixels. The HDR file must have the raw image's size, and, with --clip, its
colors clipped at 1 must come within RGBE's precision of the raw bytes (the
"clip" color model), and some must be brighter than 1. Only the Python
standard library is needed. Exits 0 if the image passes.
"""
import struct
import sys
import zlib


def read_raw(path):
    data = open(path, "rb").read()
    width, height = struct.unpack(">HH", data[:4])
    return width, height, data[4:4 + width * height * 3]


def read_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    at, idat, header = 8, b"", None
    while at < len(data):
        length, kind = struct.unpack(">I4s", data[at:at + 8])
        body = data[at + 8:at + 8 + length]
        crc = struct.unpack(">I", data[at + 8 + length:at + 12 + length])[0]
        if zlib.crc32(kind + body) & 0xFFFFFFFF != crc:
            raise ValueError(f"bad CRC in {kind!r}")
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        at += 12 + length
    width, height, depth, color = header[:4]
    if depth != 8 or color != 2:
        raise ValueError("not 8-bit RGB")
    rows = zlib.decompress(idat)
    stride = width * 3
    out = bytearray()
    previous = bytearray(stride)
    for y in range(height):
        kind = rows[y * (stride + 1)]
        line = bytearray(rows[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for x in range(stride):
            left = line[x - 3] if x >= 3 else 0
            up = previous[x]
            up_left = previous[x - 3] if x >= 3 else 0
            if kind == 0:
                guess = 0
            elif kind == 1:
                guess = left
            elif kind == 2:
                guess = up
            elif kind == 3:
                guess = (left + up) // 2
            else:
                p = left + up - up_left
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - up_left)
                guess = left if pa <= pb and pa <= pc else up if pb <= pc else up_left
            line[x] = (line[x] + guess) & 0xFF
        out += line
        previous = line
    return width, height, bytes(out)


def read_hdr(path):
    data = open(path, "rb").read()
    if not data.startswith(b"#?RADIANCE"):
        raise ValueError("not a Radiance file")
    end = data.index(b"\n\n") + 2
    if b"FORMAT=32-bit_rle_rgbe" not in data[:end]:
        raise ValueError("not RGBE")
    line_end = data.index(b"\n", end)
    parts = data[end:line_end].split()
    if parts[0] != b"-Y" or parts[2] != b"+X":
        raise ValueError("not top-to-bottom, left-to-right")
    height, width = int(parts[1]), int(parts[3])
    pixels = data[line_end + 1:]
    if len(pixels) != width * height * 4:
        raise ValueError("wrong length (only flat scanlines are read)")
    colors = []
    for at in range(0, len(pixels), 4):
        r, g, b, e = pixels[at:at + 4]
        scale = 0.0 if e == 0 else 2.0 ** (e - 136)
        colors.append(((r + 0.5) * scale if e else 0.0, (g + 0.5) * scale if e else 0.0, (b + 0.5) * scale if e else 0.0))
    return width, height, colors


def main():
    width, height, raw = read_raw(sys.argv[1])
    if sys.argv[2] == "--png":
        w, h, pixels = read_png(sys.argv[3])
        if (w, h) != (width, height) or pixels != raw:
            print("The PNG doesn't match the raw image")
            return 1
    elif sys.argv[2] == "--hdr":
        w, h, colors = read_hdr(sys.argv[3])
        if (w, h) != (width, height):
            print("The HDR file is the wrong size")
            return 1
        if "--clip" in sys.argv:
            worst = 0.0
            for index, color in enumerate(colors):
                for channel in range(3):
                    expected = raw[index * 3 + channel]
                    got = min(1.0, color[channel]) * 255.0
                    worst = max(worst, abs(got - expected))
            if worst > 2.0:
                print(f"The HDR colors are off from the raw image by up to {worst:.2f}")
                return 1
            if max(max(c) for c in colors) <= 1.0:
                print("No HDR color is brighter than 1")
                return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
