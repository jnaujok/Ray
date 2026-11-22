#!/usr/bin/env python3
"""Write the lens cutaway scenes: each built-in lens prescription built as glass
(a `lensElements` object), cut away to show its elements, with its ray fans.

    make_lens_cutaways.py [repository root]

Writes scenes/cutaways/<prescription>.ray for every prescription in
core/src/Lens.cpp, and scenes/lens_cutaway_test.ray (the double Gauss, small,
for the test suite). Each scene is framed from the lens's length and its
paraxial back focal distance, worked out here from the same table Ray reads.
Only the Python standard library is needed.
"""
import json
import math
import os
import re
import sys

ROW = re.compile(r"\{\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(TRUE|FALSE)\s*,")


def prescriptions(lens_cpp):
    """The tables, by prescription name, as lists of (radius, thickness, index, aperture)."""
    text = open(lens_cpp, encoding="latin-1").read()
    tables = {}
    for match in re.finditer(r"const CLens::SSurface (s_ao\w+)\[\] =\s*\{(.*?)\n\s*\};", text, re.S):
        tables[match.group(1)] = [(float(r[0]), float(r[1]), float(r[2]), float(r[3]))
                                  for r in ROW.findall(match.group(2))]
    names = {}
    for match in re.finditer(r'\{\s*"([a-z0-9-]+)",\s*"[^"]*",\s*(s_ao\w+)', text):
        names[match.group(1)] = tables[match.group(2)]
    return names


def paraxial(surfaces):
    """Effective focal length and back focal distance (mm), from a ray from infinity."""
    y, u, n = 1.0, 0.0, 1.0
    for index, (radius, thickness, after, _) in enumerate(surfaces):
        power = 0.0 if radius == 0.0 else (after - n) / radius
        u = (n * u - y * power) / after
        n = after
        if index + 1 < len(surfaces):
            y += u * thickness
    return -1.0 / u, -y / u


def scene(name, surfaces, size, sampler, fields):
    length = sum(s[1] for s in surfaces[:-1])
    widest = max(s[3] for s in surfaces)
    _, back = paraxial(surfaces)
    lead = max(0.5 * length, 0.4 * widest)
    span = lead + length + back                  # mm, from the rays' start to the film
    scale = 12.0 / span                          # scene units per mm: every lens 12 units long
    middle = (-lead + length + back) / 2.0 * scale
    height = 2.6 * widest * scale
    width = max(12.0, height * size[0] / size[1]) * 1.12
    distance = width / 2.0 / math.tan(math.radians(20.0))
    return {
        "format": "Ray scene",
        "version": 3,
        "settings": {"imageWidth": size[0], "imageHeight": size[1], "imageFile": name + ".out",
                     "colorDepth": 8, "maximumDepth": 12, "depthThreshold": 0.01},
        "background": [0.035, 0.04, 0.055],
        "camera": {
            "position": [middle, 0.4, -distance], "lookAt": [middle, 0.0, 0.0], "up": [0.0, 1.0, 0.0],
            "verticalAngle": 40.0 * size[1] / size[0], "horizontalAngle": 40.0,
            "aspectRatio": round(size[0] / size[1], 6), "sampler": sampler, "colorModel": "srgb",
        },
        "objects": [
            {"type": "lensElements", "prescription": name, "scale": scale, "position": [0.0, 0.0, 0.0],
             "rotation": [180.0, -90.0, 0.0], "cutaway": True, "barrel": False, "fans": True, "rays": 7,
             "fields": fields, "tubeRadius": round(0.035 / scale, 6)},
            {"type": "plane", "position": [0.0, 0.0, widest * scale + 8.0], "normal": [0.0, 0.0, -1.0],
             "color": [0.5, 0.5, 0.5],
             # Lit by nothing (ambient only), so the lens casts no shadow on it.
             "materials": [{"type": "ambient", "influence": 1.0, "color": [1.0, 1.0, 1.0],
                            "texture": {"type": "checker", "colors": [[0.05, 0.055, 0.07], [0.1, 0.11, 0.14]],
                                        "size": 1.0, "solid": False}}]},
        ],
        "lights": [
            {"type": "point", "position": [middle - 6.0, 9.0, -14.0], "color": [0.85, 0.85, 0.9]},
            {"type": "point", "position": [middle + 9.0, -4.0, -10.0], "color": [0.4, 0.4, 0.48]},
        ],
    }


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    tables = prescriptions(os.path.join(root, "core", "src", "Lens.cpp"))
    out = os.path.join(root, "scenes", "cutaways")
    os.makedirs(out, exist_ok=True)
    stratified = {"type": "stratified", "grid": 3, "seed": 0}
    for name, surfaces in tables.items():
        path = os.path.join(out, name + ".ray")
        with open(path, "w") as f:
            json.dump(scene(name, surfaces, (960, 540), stratified, [0.0, 0.7]), f, indent=4)
            f.write("\n")
        print(path)
    path = os.path.join(root, "scenes", "lens_cutaway_test.ray")
    with open(path, "w") as f:
        json.dump(scene("double-gauss-50mm", tables["double-gauss-50mm"], (320, 180),
                        {"type": "stratified", "grid": 2, "seed": 0}, [0.0, 0.7]), f, indent=4)
        f.write("\n")
    print(path)


if __name__ == "__main__":
    main()
