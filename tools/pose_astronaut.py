#!/usr/bin/env python3
"""Lower the arms of NASA's 3D Resources astronaut, so it stands as Aldrin does in AS11-40-5903.

    pose_astronaut.py <astronaut.glb> <out.glb> [--angle DEG] [--forward DEG]

NASA's astronaut model (public domain) is in a T-pose with its arms straight
out to the sides. This turns every vertex of its body mesh outside the
shoulders down about the shoulder (by --angle, default 72 degrees) and a little
forward (--forward, default 12), blending the turn over the shoulder so the
suit stays joined, and turns the normals with it. It reads the decompressed
.glb that tools/gltf_decompress.py writes (plain float accessors) and changes
nothing else. Numbers are in the model's own frame: z up, the arms along x,
the front toward -y, the shoulders at |x| 12 to 16 and z 9 to 22.
"""
import json
import math
import struct
import sys

SHOULDER_X = 13.0       # the pivot's |x|
SHOULDER_Z = 15.0       # and height
BLEND_FROM = 11.5       # |x| where the turn starts
BLEND_TO = 16.0         # and reaches its full angle
ARM_LOW = 8.0           # only above this height (below are the legs)


def main(argv):
    args = [a for a in argv if not a.startswith("--")]
    angle, forward = 72.0, 12.0
    for i, a in enumerate(argv):
        if a == "--angle":
            angle = float(argv[i + 1])
        elif a == "--forward":
            forward = float(argv[i + 1])
    if len(args) < 2 or args[1] == str(angle) or args[1] == str(forward):
        print(__doc__)
        return 1
    data = bytearray(open(args[0], "rb").read())
    length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(bytes(data[20:20 + length]))
    binary = 20 + length + 8
    mesh = gltf["meshes"][0]
    prim = mesh["primitives"][0]       # the suit; primitive 1 is the visor
    moved = 0
    for name in ("POSITION", "NORMAL"):
        if name not in prim["attributes"]:
            continue
        acc = gltf["accessors"][prim["attributes"][name]]
        pacc = gltf["accessors"][prim["attributes"]["POSITION"]]
        if acc["componentType"] != 5126 or pacc["componentType"] != 5126:
            sys.exit("decompress the model first (tools/gltf_decompress.py)")
        start = binary + gltf["bufferViews"][acc["bufferView"]]["byteOffset"] + acc.get("byteOffset", 0)
        pstart = binary + gltf["bufferViews"][pacc["bufferView"]]["byteOffset"] + pacc.get("byteOffset", 0)
        for i in range(acc["count"]):
            px, py, pz = struct.unpack_from("<3f", data, pstart + 12 * i)
            ax = abs(px)
            if ax <= BLEND_FROM or pz < ARM_LOW:
                continue
            w = min(1.0, (ax - BLEND_FROM) / (BLEND_TO - BLEND_FROM))
            w = w * w * (3 - 2 * w)
            down = math.radians(angle) * w
            fwd = math.radians(forward) * w
            side = 1.0 if px > 0 else -1.0
            x, y, z = struct.unpack_from("<3f", data, start + 12 * i)
            if name == "POSITION":
                x, z = x - side * SHOULDER_X, z - SHOULDER_Z
            # Down: in the x-z plane, the arm toward -z.
            c, s = math.cos(down), math.sin(down)
            x, z = c * x + side * s * z, -side * s * x + c * z
            # Forward: about the vertical through the shoulder, toward -y.
            c, s = math.cos(fwd), math.sin(fwd)
            x, y = c * x + side * s * y, -side * s * x + c * y
            if name == "POSITION":
                x, z = x + side * SHOULDER_X, z + SHOULDER_Z
                moved += 1
            struct.pack_into("<3f", data, start + 12 * i, x, y, z)
        if name == "POSITION":
            pts = [struct.unpack_from("<3f", data, start + 12 * i) for i in range(acc["count"])]
            acc["min"] = [min(p[k] for p in pts) for k in range(3)]
            acc["max"] = [max(p[k] for p in pts) for k in range(3)]
    text = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    while len(text) % 4:
        text += b" "
    rest = bytes(data[20 + length:])
    total = 12 + 8 + len(text) + len(rest)
    with open(args[1], "wb") as f:
        f.write(struct.pack("<4sII", b"glTF", 2, total))
        f.write(struct.pack("<II", len(text), 0x4E4F534A) + text)
        f.write(rest)
    print("%s: %d vertices turned" % (args[1], moved))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
