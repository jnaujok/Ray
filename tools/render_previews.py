#!/usr/bin/env python3
"""Write PNG previews of the reference renders, for the README.

    render_previews.py [tests/reference/renders.tar.gz] [docs/images/tests]

Each raw image in the archive (a 2-byte width, a 2-byte height, both
big-endian, then RGB bytes) becomes a PNG of the same name, shrunk by a
whole factor to at most 320 pixels wide by averaging blocks of pixels.
Only the Python standard library is needed. Run it again whenever the
reference renders are regenerated.
"""
import os
import struct
import sys
import tarfile
import zlib

MAX_WIDTH = 320


def read_raw(data):
    width, height = struct.unpack(">HH", data[:4])
    pixels = data[4:4 + width * height * 3]
    if len(pixels) != width * height * 3:
        raise ValueError("the image is shorter than its size says")
    return width, height, pixels


def shrink(width, height, pixels):
    factor = 1
    while width // factor > MAX_WIDTH:
        factor += 1
    if factor == 1:
        return width, height, pixels
    new_width, new_height = width // factor, height // factor
    out = bytearray(new_width * new_height * 3)
    area = factor * factor
    for y in range(new_height):
        for x in range(new_width):
            totals = [0, 0, 0]
            for dy in range(factor):
                row = ((y * factor + dy) * width + x * factor) * 3
                for dx in range(factor):
                    at = row + dx * 3
                    totals[0] += pixels[at]
                    totals[1] += pixels[at + 1]
                    totals[2] += pixels[at + 2]
            at = (y * new_width + x) * 3
            out[at:at + 3] = bytes((t + area // 2) // area for t in totals)
    return new_width, new_height, bytes(out)


def png(width, height, pixels):
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    rows = b"".join(b"\x00" + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b""))


def main():
    archive = sys.argv[1] if len(sys.argv) > 1 else "tests/reference/renders.tar.gz"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "docs/images/tests"
    os.makedirs(out_dir, exist_ok=True)
    with tarfile.open(archive) as tar:
        for member in sorted(tar.getmembers(), key=lambda m: m.name):
            name = os.path.basename(member.name)
            if not member.isfile() or not name.endswith(".raw"):
                continue
            width, height, pixels = shrink(*read_raw(tar.extractfile(member).read()))
            with open(os.path.join(out_dir, name[:-4] + ".png"), "wb") as out:
                out.write(png(width, height, pixels))
            print(f"{name[:-4]}.png  {width}x{height}")


if __name__ == "__main__":
    main()
