#!/usr/bin/env python3
"""Make the brick textures in scenes/textures/, for the bump test and the torture test.

    make_textures.py [scenes/textures]

Writes three 256x256 PNG images of the same brick wall (running bond, four bricks
across and eight rows down, so it tiles):

    bricks.png         the color: 8-bit RGB, each row with a different PNG filter
    bricks_normal.png  a tangent-space normal map (OpenGL convention: green up),
                       8-bit RGB, Adam7-interlaced
    bricks_height.png  the height field the normal map is made from, 16-bit gray

Between them they use most of what Ray's PNG reader must handle. Only the Python
standard library is needed, and the output is the same every time.
"""
import math
import os
import struct
import sys
import zlib

SIZE = 256
BRICK_W = 64
BRICK_H = 32
MORTAR = 4
BEVEL = 3.0


def hash01(*values):
    """A repeatable pseudo-random number in [0, 1) from some integers."""
    h = 2166136261
    for v in values:
        h ^= v & 0xFFFFFFFF
        h = (h * 16777619) & 0xFFFFFFFF
        h ^= h >> 13
        h = (h * 0x5BD1E995) & 0xFFFFFFFF
        h ^= h >> 15
    return h / 2**32


def brick_at(x, y):
    """Which brick a pixel is in, and how far it is from the brick's edge."""
    row = y // BRICK_H
    shift = (BRICK_W // 2) if row % 2 else 0
    column = ((x + shift) % SIZE) // BRICK_W
    bx = (x + shift) % BRICK_W
    by = y % BRICK_H
    # Mortar along the top and left of each brick, so the pattern tiles.
    edge = min(bx - MORTAR + 0.5, BRICK_W - bx - 0.5, by - MORTAR + 0.5, BRICK_H - by - 0.5)
    return row, column, edge


def height(x, y):
    x %= SIZE
    y %= SIZE
    row, column, edge = brick_at(x, y)
    if edge < 0:
        # Mortar: low, a little rough.
        return 0.05 * hash01(x, y, 7)
    face = min(1.0, edge / BEVEL)
    # The brick face: raised, with a slight tilt of its own and some pits.
    tilt = (hash01(row, column, 3) - 0.5) * 0.08 * ((x % BRICK_W) / BRICK_W - 0.5)
    pit = 0.12 if hash01(x // 3, y // 3, row, 11) > 0.93 else 0.0
    return 0.55 + 0.45 * face - pit + tilt


def color(x, y):
    row, column, edge = brick_at(x, y)
    if edge < 0:
        g = 0.62 + 0.08 * hash01(x, y, 5)
        return (g, g * 0.98, g * 0.93)
    tone = hash01(row, column, 1)
    base = (0.55 + 0.15 * tone, 0.24 + 0.08 * tone, 0.16 + 0.05 * hash01(row, column, 2))
    speck = 0.85 + 0.3 * hash01(x, y, 9)
    soot = 0.75 if hash01(row, column, 4) > 0.85 else 1.0
    return tuple(min(1.0, c * speck * soot) for c in base)


def normal(x, y):
    # Sobel slopes of the height field, in height units a pixel.
    def h(dx, dy):
        return height(x + dx, y + dy)
    ddx = ((h(1, -1) + 2 * h(1, 0) + h(1, 1)) - (h(-1, -1) + 2 * h(-1, 0) + h(-1, 1))) / 8.0
    ddy = ((h(-1, 1) + 2 * h(0, 1) + h(1, 1)) - (h(-1, -1) + 2 * h(0, -1) + h(1, -1))) / 8.0
    strength = 6.0
    # u runs right and v up the image, but rows run down: so +v is -row.
    nx, ny, nz = -ddx * strength, ddy * strength, 1.0
    length = math.sqrt(nx * nx + ny * ny + nz * nz)
    return (nx / length, ny / length, nz / length)


def chunk(kind, data):
    out = struct.pack(">I", len(data)) + kind + data
    return out + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def filter_row(kind, row, previous, step):
    out = bytearray([kind])
    for i, value in enumerate(row):
        left = row[i - step] if i >= step else 0
        up = previous[i]
        up_left = previous[i - step] if i >= step else 0
        predict = [0, left, up, (left + up) // 2, paeth(left, up, up_left)][kind]
        out.append((value - predict) & 0xFF)
    return out


def encode(width, height_, rows_of, color_type, depth, step, interlace=False):
    """rows_of(x0, y0, dx, dy) gives the raw rows of a pass (or the whole image)."""
    passes = [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)] \
        if interlace else [(0, 0, 1, 1)]
    raw = bytearray()
    for x0, y0, dx, dy in passes:
        rows = rows_of(x0, y0, dx, dy)
        previous = bytes(len(rows[0])) if rows else b""
        for index, row in enumerate(rows):
            raw += filter_row(index % 5, row, previous, step)
            previous = row
    header = struct.pack(">IIBBBBB", width, height_, depth, color_type, 0, 0, 1 if interlace else 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
            + chunk(b"IEND", b""))


def rows_from(pixel, channels, depth):
    def rows_of(x0, y0, dx, dy):
        rows = []
        for y in range(y0, SIZE, dy):
            row = bytearray()
            for x in range(x0, SIZE, dx):
                for value in pixel(x, y)[:channels]:
                    if depth == 16:
                        row += struct.pack(">H", max(0, min(65535, round(value * 65535))))
                    else:
                        row.append(max(0, min(255, round(value * 255))))
            rows.append(bytes(row))
        return rows
    return rows_of


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join("scenes", "textures")
    os.makedirs(folder, exist_ok=True)
    images = {
        "bricks.png": encode(SIZE, SIZE, rows_from(color, 3, 8), 2, 8, 3),
        "bricks_normal.png": encode(SIZE, SIZE, rows_from(lambda x, y: tuple((c + 1) / 2 for c in normal(x, y)), 3, 8),
                                    2, 8, 3, interlace=True),
        "bricks_height.png": encode(SIZE, SIZE, rows_from(lambda x, y: (height(x, y),), 1, 16), 0, 16, 2),
    }
    for name, data in images.items():
        with open(os.path.join(folder, name), "wb") as out:
            out.write(data)
        print(f"{name}  {len(data)} bytes")


if __name__ == "__main__":
    main()
