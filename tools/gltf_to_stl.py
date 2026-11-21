#!/usr/bin/env python3
"""Turn the meshes of a glTF 2.0 file (.gltf with its .bin, or .glb) into binary STL files.

    gltf_to_stl.py <model.gltf|model.glb> <out-dir> [--node NAME[=FILE]]... [--local]
                   [--by-material] [--scale S] [--keep-handedness]

Ray reads STL and OBJ, not glTF, so the models from NASA's 3D Resources and Poly
Haven that the scenes use are converted with this script. Only the standard
library is needed.

  --node NAME[=FILE]  write only this node (and its children), to FILE (default
                      NAME.stl); give it again for more nodes. Without --node,
                      every mesh in the default scene goes into one file named
                      after the model.
  --local             leave out the chosen node's own placement, so the mesh is
                      written about its own origin (its children keep theirs).
  --by-material       write one file per material instead (FILE-material.stl).
  --scale S           multiply every coordinate by S (default 1).
  --keep-handedness   glTF is right-handed and Ray's scenes are left-handed, so
                      by default z is negated (and the faces turned back round)
                      to keep the model from being mirrored; this leaves it.

Each triangle's normal is written from its corners. Points, lines and
primitives other than triangles, strips and fans are skipped.
"""
import json
import math
import os
import struct
import sys

COMPONENTS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
FORMATS = {5120: "b", 5121: "B", 5122: "h", 5123: "H", 5125: "I", 5126: "f"}


def load(path):
    data = open(path, "rb").read()
    if data[:4] == b"glTF":
        length = struct.unpack_from("<I", data, 8)[0]
        offset, gltf, binary = 12, None, b""
        while offset < length:
            size, kind = struct.unpack_from("<II", data, offset)
            chunk = data[offset + 8:offset + 8 + size]
            if kind == 0x4E4F534A:
                gltf = json.loads(chunk.decode("utf-8"))
            elif kind == 0x004E4942:
                binary = chunk
            offset += 8 + size
        buffers = [binary]
    else:
        gltf = json.loads(data.decode("utf-8"))
        folder = os.path.dirname(path)
        buffers = [open(os.path.join(folder, b["uri"]), "rb").read() for b in gltf["buffers"]]
    return gltf, buffers


def accessor(gltf, buffers, index):
    acc = gltf["accessors"][index]
    view = gltf["bufferViews"][acc["bufferView"]]
    buf = buffers[view.get("buffer", 0)]
    fmt = FORMATS[acc["componentType"]]
    count, width = acc["count"], COMPONENTS[acc["type"]]
    item = struct.calcsize("<" + fmt)
    stride = view.get("byteStride") or item * width
    base = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    out = []
    for i in range(count):
        values = struct.unpack_from("<%d%s" % (width, fmt), buf, base + i * stride)
        out.append(values if width > 1 else values[0])
    return out


def decode_draco(gltf, buffers, draco):
    """Points and triangle corners of a Draco-compressed primitive (needs DracoPy)."""
    try:
        import DracoPy
    except ImportError:
        sys.exit("this model is Draco-compressed: pip install DracoPy (and numpy) to convert it")
    view = gltf["bufferViews"][draco["bufferView"]]
    buf = buffers[view.get("buffer", 0)]
    start = view.get("byteOffset", 0)
    mesh = DracoPy.decode(buf[start:start + view["byteLength"]])
    points = [tuple(float(v) for v in p) for p in mesh.points.reshape(-1, 3)]
    order = [int(i) for i in mesh.faces.reshape(-1)]
    return points, order


def matmul(a, b):
    return [sum(a[r + 4 * k] * b[k + 4 * c] for k in range(4)) for c in range(4) for r in range(4)]


def node_matrix(node):
    if "matrix" in node:
        return list(node["matrix"])
    tx, ty, tz = node.get("translation", (0, 0, 0))
    qx, qy, qz, qw = node.get("rotation", (0, 0, 0, 1))
    sx, sy, sz = node.get("scale", (1, 1, 1))
    rot = [1 - 2 * (qy * qy + qz * qz), 2 * (qx * qy + qz * qw), 2 * (qx * qz - qy * qw), 0,
           2 * (qx * qy - qz * qw), 1 - 2 * (qx * qx + qz * qz), 2 * (qy * qz + qx * qw), 0,
           2 * (qx * qz + qy * qw), 2 * (qy * qz - qx * qw), 1 - 2 * (qx * qx + qy * qy), 0,
           0, 0, 0, 1]
    scale = [sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1]
    trans = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, tx, ty, tz, 1]
    return matmul(trans, matmul(rot, scale))


def apply(m, p):
    x, y, z = p
    return (m[0] * x + m[4] * y + m[8] * z + m[12],
            m[1] * x + m[5] * y + m[9] * z + m[13],
            m[2] * x + m[6] * y + m[10] * z + m[14])


def triangles(gltf, buffers, node_index, matrix, out):
    node = gltf["nodes"][node_index]
    if "mesh" in node:
        for prim in gltf["meshes"][node["mesh"]]["primitives"]:
            mode = prim.get("mode", 4)
            if mode not in (4, 5, 6):
                continue
            draco = prim.get("extensions", {}).get("KHR_draco_mesh_compression")
            if draco:
                raw, order = decode_draco(gltf, buffers, draco)
                points = [apply(matrix, p) for p in raw]
            else:
                points = [apply(matrix, p) for p in accessor(gltf, buffers, prim["attributes"]["POSITION"])]
                order = accessor(gltf, buffers, prim["indices"]) if "indices" in prim else list(range(len(points)))
            if mode == 4:
                tris = [order[i:i + 3] for i in range(0, len(order) - 2, 3)]
            elif mode == 5:
                tris = [(order[i], order[i + 1], order[i + 2]) if i % 2 == 0 else (order[i + 1], order[i], order[i + 2])
                        for i in range(len(order) - 2)]
            else:
                tris = [(order[0], order[i], order[i + 1]) for i in range(1, len(order) - 1)]
            material = prim.get("material")
            name = gltf["materials"][material].get("name", "material%d" % material) if material is not None else "default"
            out.setdefault(name, []).extend((points[a], points[b], points[c]) for a, b, c in tris)
    for child in node.get("children", []):
        triangles(gltf, buffers, child, matmul(matrix, node_matrix(gltf["nodes"][child])), out)


def write_stl(path, tris, scale, flip):
    with open(path, "wb") as f:
        f.write(b"Converted from glTF by Ray's tools/gltf_to_stl.py".ljust(80, b" "))
        f.write(struct.pack("<I", len(tris)))
        for a, b, c in tris:
            pts = [(p[0] * scale, p[1] * scale, (-p[2] if flip else p[2]) * scale) for p in (a, b, c)]
            if flip:
                pts = [pts[0], pts[2], pts[1]]
            ux, uy, uz = (pts[1][i] - pts[0][i] for i in range(3))
            vx, vy, vz = (pts[2][i] - pts[0][i] for i in range(3))
            n = (uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx)
            length = math.sqrt(sum(v * v for v in n)) or 1.0
            f.write(struct.pack("<12fH", *(v / length for v in n), *pts[0], *pts[1], *pts[2], 0))
    print("%s: %d triangles" % (path, len(tris)))


def main(argv):
    args = [a for a in argv if not a.startswith("--")]
    if len(args) < 2:
        print(__doc__)
        return 1
    source, folder = args[0], args[1]
    nodes, local, by_material, scale, flip = [], False, False, 1.0, True
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--node":
            nodes.append(argv[i + 1])
            i += 1
        elif a == "--local":
            local = True
        elif a == "--by-material":
            by_material = True
        elif a == "--scale":
            scale = float(argv[i + 1])
            i += 1
        elif a == "--keep-handedness":
            flip = False
        i += 1
    gltf, buffers = load(source)
    os.makedirs(folder, exist_ok=True)
    names = {n.get("name"): k for k, n in enumerate(gltf["nodes"])}
    identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    jobs = []
    if nodes:
        for spec in nodes:
            name, _, file = spec.partition("=")
            if name not in names:
                print("no node named %s" % name)
                return 1
            jobs.append(([names[name]], file or name + ".stl"))
    else:
        scene = gltf["scenes"][gltf.get("scene", 0)]
        jobs.append((scene["nodes"], os.path.splitext(os.path.basename(source))[0].replace(" ", "_") + ".stl"))
    for roots, file in jobs:
        groups = {}
        for root in roots:
            matrix = identity if (local and nodes) else node_matrix(gltf["nodes"][root])
            triangles(gltf, buffers, root, matrix, groups)
        if by_material:
            for material, tris in sorted(groups.items()):
                stem = os.path.splitext(file)[0]
                write_stl(os.path.join(folder, "%s-%s.stl" % (stem, material.replace(" ", "_"))), tris, scale, flip)
        else:
            write_stl(os.path.join(folder, file), [t for _, tris in sorted(groups.items()) for t in tris], scale, flip)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
