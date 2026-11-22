#!/usr/bin/env python3
"""Rewrite a Radiance HDR file with run-length encoded scanlines.

    hdr_rle.py <in.hdr> [out.hdr]

raythrower's --hdr writes flat RGBE pixels; this packs each scanline the new
Radiance way (each of the four channels in runs and literals), which keeps the
pixels exactly and shrinks images with large even areas, such as the black sky
of a baked lunar panorama, many times over. Without out.hdr it rewrites in.hdr.
"""
import sys


def read_flat(path):
    data = open(path, 'rb').read()
    end = data.index(b'\n\n') + 2
    line_end = data.index(b'\n', end)
    size = data[end:line_end].split()
    if size[0] != b'-Y' or size[2] != b'+X':
        raise SystemExit('%s: only -Y height +X width images' % path)
    height, width = int(size[1]), int(size[3])
    header = data[:end]
    pixels = data[line_end + 1:]
    if len(pixels) != width * height * 4:
        raise SystemExit('%s: not a flat RGBE image (already run-length encoded?)' % path)
    return header, width, height, pixels


def pack_channel(values):
    out = bytearray()
    i, n = 0, len(values)
    while i < n:
        run = 1
        while i + run < n and run < 127 and values[i + run] == values[i]:
            run += 1
        if run >= 3:
            out += bytes((128 + run, values[i]))
            i += run
            continue
        start = i
        while i < n and i - start < 128:
            if i + 2 < n and values[i] == values[i + 1] == values[i + 2]:
                break
            i += 1
        out.append(i - start)
        out += values[start:i]
    return out


def main(argv):
    if len(argv) not in (2, 3):
        raise SystemExit(__doc__)
    header, width, height, pixels = read_flat(argv[1])
    if not 8 <= width < 32768:
        raise SystemExit('run-length encoding needs a width from 8 to 32767')
    out = bytearray(header + b'-Y %d +X %d\n' % (height, width))
    row = width * 4
    for y in range(height):
        line = pixels[y * row:(y + 1) * row]
        out += bytes((2, 2, width >> 8, width & 255))
        for c in range(4):
            out += pack_channel(line[c::4])
    open(argv[2] if len(argv) == 3 else argv[1], 'wb').write(out)


if __name__ == '__main__':
    main(sys.argv)
