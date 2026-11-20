#!/usr/bin/env python3
"""Write the small model files Ray's tests and sample scenes use.

    make_models.py [scenes/models]

  - color_cube.stl: a puzzle cube, 3x3 colored stickers on each face with
    black gaps, as a binary STL colored the VisCAM/SolidView way (15-bit
    color in each triangle's attribute, bit 15 set, blue in the low bits).
  - magics_gem.stl: an octahedron colored the Materialise Magics way:
    "COLOR=" and a whole-model color in the header; four faces have their
    own color (bit 15 clear, red in the low bits), four take the model's.
  - ring.obj: a torus of quads with texture coordinates and normals,
    written with negative (relative) indices and with object, group,
    smoothing and material lines a reader should ignore.

Only the Python standard library is needed, and the files come out the
same every time, so they can be checked in and regenerated.
"""
import math
import os
import struct
import sys


def viscam(rgb):
    r, g, b = (int(round(c * 31)) for c in rgb)
    return 0x8000 | (r << 10) | (g << 5) | b


def magics(rgb):
    r, g, b = (int(round(c * 31)) for c in rgb)
    return r | (g << 5) | (b << 10)          # bit 15 clear: the face's own color


def write_stl(path, header, triangles):
    """triangles: (a, b, c, attribute); corners counterclockwise from outside."""
    out = bytearray(header.ljust(80, b"\0")[:80])
    out += struct.pack("<I", len(triangles))
    for a, b, c, attribute in triangles:
        u = [b[i] - a[i] for i in range(3)]
        v = [c[i] - a[i] for i in range(3)]
        n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
        length = math.sqrt(sum(x * x for x in n)) or 1.0
        out += struct.pack("<3f", *(x / length for x in n))
        for corner in (a, b, c):
            out += struct.pack("<3f", *corner)
        out += struct.pack("<H", attribute)
    with open(path, "wb") as f:
        f.write(out)


def color_cube():
    """A cube from -1 to 1 with 3x3 stickers on each face."""
    black = (0.05, 0.05, 0.05)
    # Each face: its outward axis, sign, and sticker color.
    faces = [((0, 1), (1.0, 1.0, 1.0)),      # +x white
             ((0, -1), (1.0, 0.85, 0.0)),    # -x yellow
             ((1, 1), (0.8, 0.05, 0.05)),    # +y red
             ((1, -1), (1.0, 0.45, 0.0)),    # -y orange
             ((2, 1), (0.0, 0.25, 0.8)),     # +z blue
             ((2, -1), (0.0, 0.6, 0.15))]    # -z green
    gap = 0.06
    third = 1.0 / 3.0
    lines = [0.0, gap, third - gap, third + gap, 2 * third - gap, 2 * third + gap, 1.0 - gap, 1.0]
    triangles = []
    for (axis, sign), color in faces:
        # Two axes across the face, ordered so (u x v) points outward.
        u_axis, v_axis = [(1, 2), (2, 0), (0, 1)][axis]
        if sign < 0:
            u_axis, v_axis = v_axis, u_axis
        for i in range(7):
            for j in range(7):
                sticker = (i % 2 == 1) and (j % 2 == 1)
                attribute = viscam(color if sticker else black)

                def corner(s, t):
                    p = [0.0, 0.0, 0.0]
                    p[axis] = float(sign)
                    p[u_axis] = -1.0 + 2.0 * s
                    p[v_axis] = -1.0 + 2.0 * t
                    return tuple(p)
                a = corner(lines[i], lines[j])
                b = corner(lines[i + 1], lines[j])
                c = corner(lines[i + 1], lines[j + 1])
                d = corner(lines[i], lines[j + 1])
                triangles.append((a, b, c, attribute))
                triangles.append((a, c, d, attribute))
    return triangles


def magics_gem():
    top, bottom = (0.0, 1.0, 0.0), (0.0, -1.0, 0.0)
    ring = [(1.0, 0.0, 0.0), (0.0, 0.0, 1.0), (-1.0, 0.0, 0.0), (0.0, 0.0, -1.0)]
    own = [(0.9, 0.1, 0.1), (0.1, 0.7, 0.2), (0.1, 0.3, 0.9), (0.9, 0.8, 0.1)]
    triangles = []
    for k in range(4):
        a, b = ring[k], ring[(k + 1) % 4]
        # Seen from outside, counterclockwise: top, then b, then a.
        triangles.append((top, b, a, magics(own[k])))
        triangles.append((bottom, a, b, 0x8000))   # bit 15 set: the model's color
    return triangles


def ring_obj():
    major, minor = 1.0, 0.35
    rings, sides = 32, 16
    lines = ["# A torus for Ray's tests: quads, texture coordinates, normals,",
             "# negative indices, and lines a reader ignores.",
             "mtllib ring.mtl", "o ring", "g band", "s 1", "usemtl plastic"]
    count = (rings + 1) * (sides + 1)
    for i in range(rings + 1):
        theta = 2 * math.pi * i / rings
        for j in range(sides + 1):
            phi = 2 * math.pi * j / sides
            x = (major + minor * math.cos(phi)) * math.cos(theta)
            z = (major + minor * math.cos(phi)) * math.sin(theta)
            y = minor * math.sin(phi)
            lines.append("v %.6f %.6f %.6f" % (x, y, z))
    for i in range(rings + 1):
        for j in range(sides + 1):
            lines.append("vt %.6f %.6f" % (2.0 * i / rings, float(j) / sides))
    for i in range(rings + 1):
        theta = 2 * math.pi * i / rings
        for j in range(sides + 1):
            phi = 2 * math.pi * j / sides
            lines.append("vn %.6f %.6f %.6f" % (math.cos(phi) * math.cos(theta), math.sin(phi), math.cos(phi) * math.sin(theta)))

    def ref(i, j):
        index = i * (sides + 1) + j            # 0-based
        rel = index - count                    # negative: back from the last one read
        return "%d/%d/%d" % (rel, rel, rel)
    for i in range(rings):
        for j in range(sides):
            # Counterclockwise seen from outside the tube.
            lines.append("f %s %s %s %s" % (ref(i, j), ref(i, j + 1), ref(i + 1, j + 1), ref(i + 1, j)))
    return "\n".join(lines) + "\n"


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join("scenes", "models")
    os.makedirs(folder, exist_ok=True)
    write_stl(os.path.join(folder, "color_cube.stl"), b"Ray color cube (VisCAM colors)", color_cube())
    header = b"Ray gem COLOR=" + bytes([230, 230, 235, 255]) + b" Magics colors"
    write_stl(os.path.join(folder, "magics_gem.stl"), header, magics_gem())
    with open(os.path.join(folder, "ring.obj"), "w", newline="\n") as f:
        f.write(ring_obj())
    print("wrote color_cube.stl, magics_gem.stl and ring.obj in", folder)


if __name__ == "__main__":
    main()
