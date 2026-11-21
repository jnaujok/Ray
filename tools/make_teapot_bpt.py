#!/usr/bin/env python3
"""Write the Utah teapot's Bezier patches as a .bpt file.

    make_teapot_bpt.py TeapotGeometry.js scenes/models/teapot.bpt

Martin Newell's 1975 teapot data (32 bicubic patches, public domain; the
three.js copy lists their 290 distinct control points) is read from the copy in three.js's TeapotGeometry.js
(examples/jsm/geometries/TeapotGeometry.js, release r160): the arrays
teapotPatches (16 indices a patch) and teapotVertices (x, y, z each). The
coordinates are written as they are, without Blinn's vertical scaling or
three.js's larger lid: z is up, the spout points along +x, the base is at
z = 0 and the knob reaches z = 3.15.

The .bpt format is the one the Utah teapot is usually passed around in: the
number of patches, then for each patch its degrees in u and v ("3 3") and its
(u + 1) * (v + 1) control points, one "x y z" a line, row by row (u varies
fastest). Only the Python standard library is needed.
"""
import re
import sys


def read_array(text, name):
    match = re.search(name + r"\s*=\s*\[(.*?)\];", text, re.S)
    if not match:
        raise SystemExit("no array %s in the file" % name)
    body = re.sub(r"/\*.*?\*/", "", match.group(1))
    return [float(item.replace(" ", "")) for item in body.split(",") if item.strip()]


def number(value):
    text = repr(float(value))
    return text[:-2] if text.endswith(".0") else text


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    with open(sys.argv[1], encoding="utf-8") as source:
        text = source.read()
    patches = [int(index) for index in read_array(text, "teapotPatches")]
    flat = read_array(text, "teapotVertices")
    vertices = [flat[n:n + 3] for n in range(0, len(flat), 3)]
    if len(patches) != 32 * 16 or max(patches) >= len(vertices):
        raise SystemExit("expected 32 patches, found %d on %d points" % (len(patches) // 16, len(vertices)))
    lines = ["32"]
    for patch in range(32):
        lines.append("3 3")
        for n in range(16):
            lines.append(" ".join(number(c) for c in vertices[patches[patch * 16 + n]]))
    with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as out:
        out.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
