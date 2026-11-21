#!/usr/bin/env python3
"""Write small Blender .blend files for Ray's .blend reader tests.

    make_blend_fixtures.py [scenes/models]

Ray can't run Blender, so this writes .blend files the way Blender does: a
header, blocks of structures with their old memory addresses, and a
structure catalog (SDNA) naming every field, in each of the two layouts
Blender has used for meshes:

  blend_legacy.blend   Blender 2.93's layout: MVert, MPoly, MLoop and
                       MLoopUV arrays, and each object's world matrix
                       (obmat). A cube "Cube" (two materials, red and blue),
                       moved, turned 90 degrees about z and scaled by 2, and
                       a triangle "Tri" whose object overrides its material
                       with a green one.
  blend_modern.blend   Blender 4.2's layout: a "position" float3 attribute,
                       face offsets, a ".corner_vert" attribute, UV maps as
                       float2 corner attributes (the second one active), a
                       "material_index" face attribute, and no world matrix,
                       so the reader builds it from location, rotation, scale
                       and the parent: a pentagon "Base" and, parented to
                       it, a tetrahedron "Child" turned 90 degrees about x.

and each again gzip-compressed (_gz) and Zstandard-compressed in Blender's
manner, several frames (_zst). The numbers are fixed, so the files are the
same every time; format_test checks what Ray reads from them.
"""
import gzip
import math
import os
import struct
import sys

PTR = 8


class Writer:
    def __init__(self, version):
        self.version = version
        self.types = []          # (name, size)
        self.structs = []        # (type, [(type, name)])
        self.names = []
        self.blocks = []
        self.next = 0x10000

    def type(self, name, size):
        for i, (n, s) in enumerate(self.types):
            if n == name:
                return i
        self.types.append((name, size))
        return len(self.types) - 1

    def name(self, n):
        if n not in self.names:
            self.names.append(n)
        return self.names.index(n)

    def struct(self, name, fields):
        size = 0
        for t, n in fields:
            count = 1
            for part in n.split("[")[1:]:
                count *= int(part.split("]")[0])
            if n.startswith("*") or n.startswith("(*"):
                size += PTR * count
            else:
                size += dict(self.types)[t] * count
        self.type(name, size)
        self.structs.append((name, fields))
        return size

    def address(self):
        self.next += 0x1000
        return self.next

    def block(self, code, data, sdna=0, count=1, address=None):
        address = address or self.address()
        self.blocks.append((code, data, address, sdna, count))
        return address

    def sdna_index(self, name):
        return [s[0] for s in self.structs].index(name)

    def dna(self):
        for name, fields in self.structs:
            for t, n in fields:
                self.name(n)
        out = b"SDNA" + b"NAME" + struct.pack("<i", len(self.names))
        out += b"".join(n.encode() + b"\0" for n in self.names)
        out += b"\0" * (-len(out) % 4)
        out += b"TYPE" + struct.pack("<i", len(self.types))
        out += b"".join(t.encode() + b"\0" for t, s in self.types)
        out += b"\0" * (-len(out) % 4)
        out += b"TLEN" + b"".join(struct.pack("<h", s) for t, s in self.types)
        out += b"\0" * (-len(out) % 4)
        out += b"STRC" + struct.pack("<i", len(self.structs))
        for name, fields in self.structs:
            out += struct.pack("<hh", self.type(name, 0), len(fields))
            for t, n in fields:
                out += struct.pack("<hh", self.type(t, 0), self.name(n))
        return out

    def write(self):
        out = b"BLENDER-v" + self.version.encode()
        # The catalog goes in first only so a damaged-file test can find it;
        # Blender writes it last, and the reader doesn't care.
        blocks = self.blocks + [(b"DNA1", self.dna(), self.address(), 0, 1)]
        for code, data, address, sdna, count in blocks:
            out += code + struct.pack("<iQii", len(data), address, sdna, count) + data
        out += b"ENDB" + struct.pack("<iQii", 0, 0, 0, 0)
        return out


def base_types(w):
    for name, size in (("char", 1), ("short", 2), ("int", 4), ("float", 4), ("void", 0)):
        w.type(name, size)
    w.struct("ID", [("void", "*next"), ("void", "*prev"), ("char", "name[66]"), ("char", "_pad[6]")])
    w.struct("CustomDataLayer", [("int", "type"), ("int", "offset"), ("int", "flag"), ("int", "active"),
                                 ("int", "active_rnd"), ("int", "active_clone"), ("int", "active_mask"), ("int", "uid"),
                                 ("char", "name[64]"), ("void", "*data")])
    w.struct("CustomData", [("CustomDataLayer", "*layers"), ("int", "totlayer"), ("int", "maxlayer")])
    w.struct("Material", [("ID", "id"), ("float", "r"), ("float", "g"), ("float", "b"), ("float", "a")])


def id_bytes(name):
    n = name.encode()
    return struct.pack("<QQ", 0, 0) + n + b"\0" * (66 - len(n)) + b"\0" * 6


def custom_data(w, layers):
    """layers: (type, name, active, data address, count) → the CustomData bytes."""
    if not layers:
        return struct.pack("<Qii", 0, 0, 0)
    body = b""
    for typ, name, active, address in layers:
        n = name.encode()
        body += struct.pack("<8i", typ, 0, 0, active, 0, 0, 0, 0) + n + b"\0" * (64 - len(n)) + struct.pack("<Q", address)
    addr = w.block(b"DATA", body, w.sdna_index("CustomDataLayer"), len(layers))
    return struct.pack("<Qii", addr, len(layers), len(layers))


def material(w, name, rgb):
    return w.block(b"MA\0\0", id_bytes("MA" + name) + struct.pack("<4f", rgb[0], rgb[1], rgb[2], 1.0), w.sdna_index("Material"))


def pointer_array(w, addresses):
    return w.block(b"DATA", b"".join(struct.pack("<Q", a) for a in addresses), 0, len(addresses))


def euler_matrix(loc, rot, size):
    """Blender's XYZ Euler: R = Rz Ry Rx; column-major 4x4."""
    cx, sx = math.cos(rot[0]), math.sin(rot[0])
    cy, sy = math.cos(rot[1]), math.sin(rot[1])
    cz, sz = math.cos(rot[2]), math.sin(rot[2])
    rx = [[1, 0, 0], [0, cx, -sx], [0, sx, cx]]
    ry = [[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]]
    rz = [[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]]

    def mul(a, b):
        return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
    r = mul(rz, mul(ry, rx))
    m = []
    for c in range(3):
        m += [r[0][c] * size[c], r[1][c] * size[c], r[2][c] * size[c], 0.0]
    return m + [loc[0], loc[1], loc[2], 1.0]


CUBE_V = [(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1), (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]
CUBE_F = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
SQUARE_UV = [(0, 0), (1, 0), (1, 1), (0, 1)]


def legacy():
    w = Writer("293")
    base_types(w)
    w.struct("MVert", [("float", "co[3]"), ("char", "flag"), ("char", "bweight"), ("char", "_pad[2]")])
    w.struct("MPoly", [("int", "loopstart"), ("int", "totloop"), ("short", "mat_nr"), ("char", "flag"), ("char", "_pad")])
    w.struct("MLoop", [("int", "v"), ("int", "e")])
    w.struct("MLoopUV", [("float", "uv[2]"), ("int", "flag")])
    w.struct("Mesh", [("ID", "id"), ("Material", "**mat"), ("CustomData", "vdata"), ("CustomData", "edata"),
                      ("CustomData", "pdata"), ("CustomData", "ldata"), ("int", "totvert"), ("int", "totedge"),
                      ("int", "totpoly"), ("int", "totloop"), ("short", "totcol"), ("char", "_pad[6]")])
    w.struct("Object", [("ID", "id"), ("void", "*data"), ("Object", "*parent"), ("Material", "**mat"), ("char", "*matbits"),
                        ("short", "type"), ("short", "totcol"), ("int", "rotmode"), ("float", "loc[3]"), ("float", "rot[3]"),
                        ("float", "size[3]"), ("float", "quat[4]"), ("float", "obmat[4][4]"), ("float", "parentinv[4][4]")])
    red = material(w, "Red", (0.8, 0.1, 0.1))
    blue = material(w, "Blue", (0.1, 0.2, 0.9))
    green = material(w, "Green", (0.1, 0.7, 0.2))

    def mesh(name, verts, faces, uvs, mats, mat_list):
        vaddr = w.block(b"DATA", b"".join(struct.pack("<3fbb2x", *v, 0, 0) for v in verts), w.sdna_index("MVert"), len(verts))
        loops, polys, uv = [], b"", b""
        for i, f in enumerate(faces):
            polys += struct.pack("<iihbx", len(loops), len(f), mats[i], 0)
            for k, v in enumerate(f):
                uv += struct.pack("<2fi", *uvs[k % len(uvs)], 0)
                loops.append(v)
        paddr = w.block(b"DATA", polys, w.sdna_index("MPoly"), len(faces))
        laddr = w.block(b"DATA", b"".join(struct.pack("<ii", v, 0) for v in loops), w.sdna_index("MLoop"), len(loops))
        uaddr = w.block(b"DATA", uv, w.sdna_index("MLoopUV"), len(loops))
        body = id_bytes("ME" + name) + struct.pack("<Q", pointer_array(w, mat_list))
        body += custom_data(w, [(0, "", 0, vaddr)]) + custom_data(w, [])
        body += custom_data(w, [(25, "", 0, paddr)]) + custom_data(w, [(16, "UVMap", 0, uaddr), (26, "", 0, laddr)])
        body += struct.pack("<4ih6x", len(verts), 0, len(faces), len(loops), len(mat_list))
        return w.block(b"ME\0\0", body, w.sdna_index("Mesh"))

    def obj(name, data, loc, rot, size, mats=(), bits=b""):
        matrix = euler_matrix(loc, rot, size)
        mat_addr = pointer_array(w, list(mats)) if mats else 0
        bits_addr = w.block(b"DATA", bits, 0, len(bits)) if bits else 0
        body = id_bytes("OB" + name) + struct.pack("<QQQQ", data, 0, mat_addr, bits_addr)
        body += struct.pack("<hhi", 1, len(mats), 1) + struct.pack("<3f", *loc) + struct.pack("<3f", *rot) + struct.pack("<3f", *size)
        body += struct.pack("<4f", 1, 0, 0, 0) + struct.pack("<16f", *matrix) + struct.pack("<16f", *euler_matrix((0, 0, 0), (0, 0, 0), (1, 1, 1)))
        w.block(b"OB\0\0", body, w.sdna_index("Object"))

    cube = mesh("Cube", CUBE_V, CUBE_F, SQUARE_UV, [0, 0, 1, 1, 1, 1], [red, blue])
    tri = mesh("Tri", [(0, 0, 0), (1, 0, 0), (0, 1, 0)], [(0, 1, 2)], [(0, 0), (1, 0), (0, 1)], [0], [red])
    obj("Cube", cube, (1.0, 2.0, 3.0), (0.0, 0.0, math.pi / 2), (2.0, 2.0, 2.0))
    obj("Tri", tri, (0.0, 0.0, -5.0), (0.0, 0.0, 0.0), (1.0, 1.0, 1.0), mats=[green], bits=b"\x01")
    return w.write()


def modern():
    w = Writer("402")
    base_types(w)
    w.struct("Mesh", [("ID", "id"), ("Material", "**mat"), ("int", "totvert"), ("int", "totedge"), ("int", "totpoly"),
                      ("int", "totloop"), ("int", "*poly_offset_indices"), ("CustomData", "vdata"), ("CustomData", "edata"),
                      ("CustomData", "pdata"), ("CustomData", "ldata"), ("short", "totcol"), ("char", "_pad[6]")])
    w.struct("Object", [("ID", "id"), ("void", "*data"), ("Object", "*parent"), ("Material", "**mat"), ("char", "*matbits"),
                        ("short", "type"), ("short", "totcol"), ("int", "rotmode"), ("float", "loc[3]"), ("float", "rot[3]"),
                        ("float", "size[3]"), ("float", "quat[4]"), ("float", "parentinv[4][4]")])
    gold = material(w, "Gold", (0.9, 0.7, 0.2))
    grey = material(w, "Grey", (0.5, 0.5, 0.5))

    def mesh(name, verts, faces, mats, uv_second):
        pos = w.block(b"DATA", b"".join(struct.pack("<3f", *v) for v in verts), 0, len(verts))
        offsets, corners = [0], []
        for f in faces:
            corners += list(f)
            offsets.append(len(corners))
        off = w.block(b"DATA", b"".join(struct.pack("<i", o) for o in offsets), 0, len(offsets))
        cv = w.block(b"DATA", b"".join(struct.pack("<i", c) for c in corners), 0, len(corners))
        uv1 = w.block(b"DATA", b"".join(struct.pack("<2f", 0.0, 0.0) for c in corners), 0, len(corners))
        uv2 = w.block(b"DATA", b"".join(struct.pack("<2f", *uv_second(i)) for i, c in enumerate(corners)), 0, len(corners))
        hidden = w.block(b"DATA", b"".join(struct.pack("<2f", 9.0, 9.0) for c in corners), 0, len(corners))
        mi = w.block(b"DATA", b"".join(struct.pack("<i", m) for m in mats), 0, len(mats))
        body = id_bytes("ME" + name) + struct.pack("<Q", pointer_array(w, [gold, grey]))
        body += struct.pack("<4i", len(verts), 0, len(faces), len(corners)) + struct.pack("<Q", off)
        body += custom_data(w, [(48, "position", 0, pos)]) + custom_data(w, [])
        body += custom_data(w, [(11, "material_index", 0, mi)])
        # Two UV maps with the second active (active counts from the first
        # layer of the type), and Blender's own hidden float2 after them.
        body += custom_data(w, [(11, ".corner_vert", 0, cv), (49, "UVMap", 1, uv1), (49, "Second", 1, uv2), (49, ".hidden", 1, hidden)])
        body += struct.pack("<h6x", 2)
        return w.block(b"ME\0\0", body, w.sdna_index("Mesh"))

    def obj(name, data, loc, rot, size, parent=0):
        body = id_bytes("OB" + name) + struct.pack("<QQQQ", data, parent, 0, 0)
        body += struct.pack("<hhi", 1, 0, 1) + struct.pack("<3f", *loc) + struct.pack("<3f", *rot) + struct.pack("<3f", *size)
        body += struct.pack("<4f", 1, 0, 0, 0) + struct.pack("<16f", *euler_matrix((0, 0, 0), (0, 0, 0), (1, 1, 1)))
        return w.block(b"OB\0\0", body, w.sdna_index("Object"))

    pent = [(math.cos(2 * math.pi * i / 5), math.sin(2 * math.pi * i / 5), 0.0) for i in range(5)]
    base_mesh = mesh("Pentagon", pent, [(0, 1, 2, 3, 4)], [0], lambda i: (i / 4.0, 0.5))
    tet = [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]
    tet_mesh = mesh("Tetra", tet, [(0, 2, 1), (0, 1, 3), (1, 2, 3), (2, 0, 3)], [1, 1, 0, 1], lambda i: (0.25, 0.75))
    base = obj("Base", base_mesh, (0.0, 0.0, 1.0), (0.0, 0.0, 0.0), (1.0, 1.0, 1.0))
    obj("Child", tet_mesh, (0.0, 0.0, 2.0), (math.pi / 2, 0.0, 0.0), (1.0, 1.0, 1.0), parent=base)
    return w.write()


def zstd_frames(data):
    import zstandard
    cctx = zstandard.ZstdCompressor(level=3, write_content_size=True)
    frames = [cctx.compress(data[i:i + 1024]) for i in range(0, len(data), 1024)]
    seek = b"".join(struct.pack("<II", len(f), min(1024, len(data) - i * 1024)) for i, f in enumerate(frames))
    seek += struct.pack("<IBI", len(frames), 0, 0x8F92EAB1)
    return b"".join(frames) + struct.pack("<II", 0x184D2A5E, len(seek)) + seek


def main(argv):
    out = argv[0] if argv else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "scenes", "models")
    for name, data in (("blend_legacy", legacy()), ("blend_modern", modern())):
        open(os.path.join(out, name + ".blend"), "wb").write(data)
        open(os.path.join(out, name + "_gz.blend"), "wb").write(gzip.compress(data, 9, mtime=0))
        open(os.path.join(out, name + "_zst.blend"), "wb").write(zstd_frames(data))
        print(name, len(data))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
