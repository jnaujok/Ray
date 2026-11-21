#!/usr/bin/env python3
"""Write a Draco-compressed glTF 2.0 binary (.glb) back out uncompressed, so Ray can read it.

    gltf_decompress.py <in.glb> <out.glb>

Ray's glTF reader (core/src/GltfFile.cpp) reads plain accessors but not
KHR_draco_mesh_compression, which would need Google's Draco decoder. NASA's 3D
Resources models are Draco-compressed, so the scenes use copies decompressed by
this script: every Draco primitive's POSITION, NORMAL, TEXCOORD_0 and indices
are decoded with DracoPy (pip install DracoPy numpy) and appended to the binary
chunk as ordinary float and uint32 accessors, the extension is dropped from the
primitives and from extensionsUsed and extensionsRequired, and everything else
(nodes, materials, images, textures) is kept as it was. Draco's quantization is
what the decoder returns, so the geometry is exactly what any glTF viewer shows.
"""
import json
import struct
import sys


def main(argv):
    if len(argv) != 2:
        print(__doc__)
        return 1
    try:
        import DracoPy
    except ImportError:
        sys.exit("needs DracoPy: pip install DracoPy numpy")
    data = open(argv[0], "rb").read()
    if data[:4] != b"glTF":
        sys.exit("%s isn't a .glb file" % argv[0])
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
    out = bytearray(binary)
    views = gltf["bufferViews"]
    accessors = gltf["accessors"]
    decoded = 0

    def append(raw, target=None):
        while len(out) % 4:
            out.append(0)
        view = {"buffer": 0, "byteOffset": len(out), "byteLength": len(raw)}
        if target:
            view["target"] = target
        out.extend(raw)
        views.append(view)
        return len(views) - 1

    for mesh in gltf.get("meshes", []):
        for prim in mesh["primitives"]:
            draco = prim.get("extensions", {}).get("KHR_draco_mesh_compression")
            if not draco:
                continue
            view = views[draco["bufferView"]]
            start = view.get("byteOffset", 0)
            decoded_mesh = DracoPy.decode(bytes(binary[start:start + view["byteLength"]]))
            columns = {"POSITION": (decoded_mesh.points, 3)}
            if getattr(decoded_mesh, "normals", None) is not None and len(decoded_mesh.normals):
                columns["NORMAL"] = (decoded_mesh.normals, 3)
            if getattr(decoded_mesh, "tex_coord", None) is not None and len(decoded_mesh.tex_coord):
                columns["TEXCOORD_0"] = (decoded_mesh.tex_coord, 2)
            for name, index in prim["attributes"].items():
                if name not in columns:
                    continue
                values, width = columns[name]
                flat = [float(v) for v in values.reshape(-1)]
                acc = accessors[index]
                acc["bufferView"] = append(struct.pack("<%df" % len(flat), *flat), 34962)
                acc["byteOffset"] = 0
                acc["componentType"] = 5126
                acc["count"] = len(flat) // width
                acc.pop("normalized", None)
                if name == "POSITION":
                    pts = values.reshape(-1, 3)
                    acc["min"] = [float(v) for v in pts.min(axis=0)]
                    acc["max"] = [float(v) for v in pts.max(axis=0)]
            for name in [n for n in prim["attributes"] if n not in columns]:
                del prim["attributes"][name]
            faces = [int(i) for i in decoded_mesh.faces.reshape(-1)]
            index = prim.get("indices")
            if index is None:
                accessors.append({"type": "SCALAR"})
                index = prim["indices"] = len(accessors) - 1
            acc = accessors[index]
            acc.update({"bufferView": append(struct.pack("<%dI" % len(faces), *faces), 34963), "byteOffset": 0,
                        "componentType": 5125, "count": len(faces), "type": "SCALAR"})
            acc.pop("min", None)
            acc.pop("max", None)
            del prim["extensions"]["KHR_draco_mesh_compression"]
            if not prim["extensions"]:
                del prim["extensions"]
            decoded += 1
    for key in ("extensionsUsed", "extensionsRequired"):
        if key in gltf:
            gltf[key] = [e for e in gltf[key] if e != "KHR_draco_mesh_compression"]
            if not gltf[key]:
                del gltf[key]
    while len(out) % 4:
        out.append(0)
    gltf["buffers"] = [{"byteLength": len(out)}]
    text = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    while len(text) % 4:
        text += b" "
    total = 12 + 8 + len(text) + 8 + len(out)
    with open(argv[1], "wb") as f:
        f.write(struct.pack("<4sII", b"glTF", 2, total))
        f.write(struct.pack("<II", len(text), 0x4E4F534A) + text)
        f.write(struct.pack("<II", len(out), 0x004E4942) + bytes(out))
    print("%s: %d primitives decoded, %d bytes" % (argv[1], decoded, total))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
