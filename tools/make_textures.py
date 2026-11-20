#!/usr/bin/env python3
"""Make the textures in scenes/textures/: bricks for the bump test and the torture test, and a sky
for the environment test and the torture test.

    make_textures.py [scenes/textures]

Writes three 256x256 PNG images of the same brick wall (running bond, four bricks
across and eight rows down, so it tiles):

    bricks.png         the color: 8-bit RGB, each row with a different PNG filter
    bricks_normal.png  a tangent-space normal map (OpenGL convention: green up),
                       8-bit RGB, Adam7-interlaced
    bricks_height.png  the height field the normal map is made from, 16-bit gray

Between them they use most of what Ray's PNG reader must handle.

It also writes sky.hdr, a 512x256 equirectangular (latitude-longitude) picture of an
afternoon sky, for an environment map: a Radiance HDR file, run-length encoded, whose sun
is far brighter than white. Its middle column looks along +z; up is +y. Blue overhead,
pale at the horizon, with clouds, a hazy ridge of hills and a sandy ground.

Only the Python standard library is needed, and the output is the same every time.
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


SKY_W = 512
SKY_H = 256
SUN_AZIMUTH = math.radians(-50.0)     # longitude: 0 is +z, positive toward +x
SUN_ELEVATION = math.radians(38.0)


def sun_direction():
    """The direction toward the sun (x, y, z), matching the sky's mapping."""
    c = math.cos(SUN_ELEVATION)
    return (math.sin(SUN_AZIMUTH) * c, math.sin(SUN_ELEVATION), math.cos(SUN_AZIMUTH) * c)


def value_noise(x, y, seed, period):
    """Smooth repeatable noise in [0, 1) on a lattice that wraps every `period` cells across x."""
    x0 = math.floor(x)
    y0 = math.floor(y)
    fx = x - x0
    fy = y - y0
    fx = fx * fx * (3 - 2 * fx)
    fy = fy * fy * (3 - 2 * fy)

    def corner(i, j):
        return hash01((x0 + i) % period, y0 + j, seed)
    top = corner(0, 0) * (1 - fx) + corner(1, 0) * fx
    bottom = corner(0, 1) * (1 - fx) + corner(1, 1) * fx
    return top * (1 - fy) + bottom * fy


def fbm(x, y, seed, period, octaves=5):
    total = 0.0
    scale = 1
    weight = 0.5
    for octave in range(octaves):
        total += weight * value_noise(x * scale, y * scale, seed + octave, period * scale)
        scale *= 2
        weight *= 0.5
    return total


def sky_color(column, row):
    longitude = ((column + 0.5) / SKY_W - 0.5) * 2 * math.pi
    elevation = math.pi / 2 - (row + 0.5) / SKY_H * math.pi
    c = math.cos(elevation)
    d = (math.sin(longitude) * c, math.sin(elevation), math.cos(longitude) * c)
    sun = sun_direction()
    toward_sun = max(-1.0, min(1.0, sum(a * b for a, b in zip(d, sun))))
    angle = math.acos(toward_sun)

    # Hills: a low ridge, a degree or two high, all round the horizon.
    u = (column + 0.5) / SKY_W
    ridge = math.radians(1.3 + 0.7 * math.sin(u * 2 * math.pi * 3 + 1.0) + 0.4 * math.sin(u * 2 * math.pi * 7 + 0.3)
                         + 0.2 * math.sin(u * 2 * math.pi * 17))
    if elevation < 0:
        # Sandy ground, darker straight down, with a little mottling.
        depth = -elevation / (math.pi / 2)
        mottle = 0.85 + 0.3 * fbm(u * 64, (row / SKY_H) * 32, 11, 64, 4)
        base = (0.42, 0.36, 0.27)
        fade = 1.0 - 0.45 * depth
        return tuple(channel * fade * mottle for channel in base)
    if elevation < ridge:
        # Hazy hills, bluer and paler toward their tops.
        t = elevation / ridge
        near = (0.36, 0.41, 0.40)
        far = (0.62, 0.68, 0.76)
        return tuple(a + (b - a) * t for a, b in zip(near, far))

    # Sky: deep blue overhead, pale at the horizon, brighter toward the sun.
    h = math.sin(elevation)
    t = h ** 0.45
    horizon = (0.80, 0.86, 0.95)
    zenith = (0.16, 0.32, 0.78)
    color = [a + (b - a) * t for a, b in zip(horizon, zenith)]
    glow = 0.35 * math.exp(-angle / 0.35) + 0.08 * (1 + toward_sun) / 2
    color = [channel + glow * warm for channel, warm in zip(color, (1.0, 0.92, 0.75))]

    # Clouds: soft patches on a flat layer overhead, so they crowd together and flatten toward
    # the horizon, where they fade into the haze.
    reach = 1.0 / max(h, 0.02)
    cloud = fbm(d[0] * reach * 1.6 + 100.0, d[2] * reach * 1.6 + 100.0, 23, 1 << 20, 5)
    haze = min(1.0, h / 0.12)
    cover = max(0.0, min(1.0, (cloud - 0.5) * 3.5)) * haze * haze
    lit = 1.05 + 0.6 * math.exp(-angle / 0.6)
    color = [channel * (1 - cover) + lit * shade * cover for channel, shade in zip(color, (0.95, 0.95, 0.97))]

    # The sun: a disc half a degree across, far brighter than white.
    if angle < math.radians(1.2):
        disc = 1.0 if angle < math.radians(0.6) else (math.radians(1.2) - angle) / math.radians(0.6)
        color = [channel + disc * 60.0 * warm for channel, warm in zip(color, (1.0, 0.95, 0.85))]
    return tuple(color)


def to_rgbe(red, green, blue):
    largest = max(red, green, blue)
    if largest < 1e-32:
        return (0, 0, 0, 0)
    mantissa, exponent = math.frexp(largest)
    scale = mantissa * 256.0 / largest
    return (int(red * scale), int(green * scale), int(blue * scale), exponent + 128)


def rle_channel(values):
    """New-style Radiance run-length encoding of one channel of a scanline."""
    out = bytearray()
    literal = bytearray()
    index = 0

    def flush():
        while literal:
            part = literal[:128]
            out.append(len(part))
            out.extend(part)
            del literal[:128]
    while index < len(values):
        run = 1
        while index + run < len(values) and run < 127 and values[index + run] == values[index]:
            run += 1
        if run >= 4:
            flush()
            out.append(128 + run)
            out.append(values[index])
            index += run
        else:
            literal.append(values[index])
            index += 1
    flush()
    return bytes(out)


def encode_hdr(width, height_, pixel):
    out = bytearray(b"#?RADIANCE\n# Made by tools/make_textures.py\nFORMAT=32-bit_rle_rgbe\n\n")
    out += f"-Y {height_} +X {width}\n".encode("ascii")
    for row in range(height_):
        rgbe = [to_rgbe(*pixel(column, row)) for column in range(width)]
        out += bytes((2, 2, width >> 8, width & 0xFF))
        for channel in range(4):
            out += rle_channel([value[channel] for value in rgbe])
    return bytes(out)


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join("scenes", "textures")
    os.makedirs(folder, exist_ok=True)
    images = {
        "bricks.png": encode(SIZE, SIZE, rows_from(color, 3, 8), 2, 8, 3),
        "bricks_normal.png": encode(SIZE, SIZE, rows_from(lambda x, y: tuple((c + 1) / 2 for c in normal(x, y)), 3, 8),
                                    2, 8, 3, interlace=True),
        "bricks_height.png": encode(SIZE, SIZE, rows_from(lambda x, y: (height(x, y),), 1, 16), 0, 16, 2),
        "sky.hdr": encode_hdr(SKY_W, SKY_H, sky_color),
    }
    for name, data in images.items():
        with open(os.path.join(folder, name), "wb") as out:
            out.write(data)
        print(f"{name}  {len(data)} bytes")


if __name__ == "__main__":
    main()
