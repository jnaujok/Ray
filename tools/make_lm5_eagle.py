#!/usr/bin/env python3
"""Rebuild NASA's Apollo Lunar Module model as Ray's own LM-5 Eagle.

    make_lm5_eagle.py [scenes/models/apollo/apollo_lunar_module.glb] [scenes/models/apollo/lm5_eagle.glb]

NASA's model (NASA 3D Resources, public domain) has generic Maya names
(blinn1SG.002, polySurfac, ...). This script decodes it (its Draco
compression, with DracoPy: pip install DracoPy numpy) and writes an
uncompressed glTF 2.0 binary whose primitives are named for what they are, so
a scene can give each part its own materials with a mesh's "materialMap":

    kapton_blanket    the descent stage's quadrant blankets: copper-red Kapton
    kapton_gold       the footpads and the lower primary struts: bright gold foil
    silver_foil       the upper primary struts: silver aluminized foil
    kapton_mesa       the MESA bay panel
    black_blanket     the descent stage's black rim bands and the blanket
                      between the stages
    dps_bell          the descent engine bell
    porch_aluminum    the porch, ladder and secondary struts
    inconel_dark      the black trusses and RCS housings
    white_paint       the dishes and other white-painted fittings
    al2024_anodized   the ascent stage's silver panels: the front face around
                      the windows and hatch, and the tops
    al5056_anodized   the ascent stage's beige panels: the sides and the aft
                      equipment bay
    al_olive          the faceted tank fairing on the +x side: darker
                      olive-gray metal, as AS11-40-5927 shows it
    pyromark_black    the ascent stage's black panels: the other tank fairing,
                      the lower forward panels and the low aft panels
    window_quartz     the windows

NASA's descent foil is one material, so it is split by place: faces low
enough to be footpads (y under 0.35 model units) and faces outside the
descent stage (beyond 1.95 from the axis) are the struts, silver above
y = 1.25 and gold below;
the rest (with the main body, pCylinder1) are the copper-red
blankets. The ascent
stage's skin is one mesh in NASA's model, so it is split face by face: a face
facing aft low on the stage (nz < -0.5, y below 3.0) is Pyromark, one facing
the sides (|nx| > 0.6) or aft is 5056, one facing forward low on the stage
(nz > 0.5, y below 2.95) is Pyromark, and the rest 2024. The windows share a material with a side panel;
forward-facing faces of it high on the stage (nz > 0.4, y above 3.4) are the
windows and the rest 5056. NASA's model has no stowed S-band erectable
antenna, so there is nothing to take off for the Apollo 11 EVA.

Every part gets smooth normals worked out from its faces up to a 45 degree
crease (a face's corners at one place share a normal summed, by area, from
the faces there within 45 degrees of it; faces NASA wound against its own
normals are turned round first), so the engine bell
and the tanks are round and the panels' edges stay sharp. The model is scaled
by 1.4 to meters (about 7 m tall, as the real LM with its probes) and stood on
y = 0, centered in x and z. glTF's +z is the LM's front (+Z, the hatch);
Ray reads glTF mirrored in z, so in a Ray scene the front faces -z.
"""
import json
import struct
import sys

import numpy as np

try:
    import DracoPy
except ImportError:
    sys.exit("needs DracoPy: pip install DracoPy numpy")

SCALE = 1.4
CREASE_DEGREES = 45.0

# Linear base colors for viewers that read glTF materials; Ray's prefab gives
# each part its own materials in a materialMap instead.
PART_COLORS = {
    'kapton_blanket': (0.43, 0.11, 0.035), 'kapton_gold': (0.70, 0.37, 0.045), 'silver_foil': (0.70, 0.72, 0.74),
    'kapton_mesa': (0.79, 0.33, 0.03), 'black_blanket': (0.01, 0.01, 0.012), 'al_olive': (0.19, 0.19, 0.15),
    'dps_bell': (0.04, 0.047, 0.054), 'porch_aluminum': (0.39, 0.42, 0.45), 'inconel_dark': (0.02, 0.022, 0.024),
    'white_paint': (0.76, 0.78, 0.79), 'al2024_anodized': (0.48, 0.51, 0.55), 'al5056_anodized': (0.56, 0.51, 0.37),
    'pyromark_black': (0.009, 0.0097, 0.011), 'window_quartz': (0.003, 0.006, 0.01),
}

BY_MATERIAL = {
    'blinn1SG.002': 'kapton_blanket', 'blinn9SG.001': 'kapton_gold', 'blinn8SG.001': 'kapton_blanket',
    'blinn12SG.001': 'kapton_mesa', 'blinn5SG.002': 'dps_bell', 'blinn4SG.002': 'porch_aluminum',
    'blinn6SG.001': 'inconel_dark', 'blinn7SG.002': 'white_paint', 'blinn10SG.001': 'white_paint',
    'blinn2SG.002': 'al2024_anodized', 'blinn3SG.001': 'al2024_anodized', 'initialShadingGr.001': 'al2024_anodized',
}
BY_NODE = {
    ('blinn2SG.002', 'pCube2'): 'al5056_anodized',            # the aft equipment bay
    ('initialShadingGr.001', 'pCylinder9'): 'al_olive',        # the tank fairings
    ('initialShadingGr.001', 'pCylinde10'): 'pyromark_black',
}
for _node in ('group31 10', 'group31 11', 'group31 12', 'group31 13', 'group31 14', 'group31 g9'):
    BY_NODE[('initialShadingGr.001', _node)] = 'black_blanket'


def face_part(material, node, normal, center):
    if (material, node) in BY_NODE:
        return BY_NODE[(material, node)]
    if material == 'blinn1SG.002':
        radius = max(abs(center[0]), abs(center[2]))
        if center[1] < 0.35:
            return 'kapton_gold'
        if radius > 1.95:
            return 'silver_foil' if center[1] > 1.25 else 'kapton_gold'
        return 'kapton_blanket'
    if node == 'polySurfac' and material == 'initialShadingGr.001':
        if normal[2] < -0.5 and center[1] < 3.0:
            return 'pyromark_black'
        if abs(normal[0]) > 0.6 or normal[2] < -0.5:
            return 'al5056_anodized'
        if normal[2] > 0.5 and center[1] < 2.95:
            return 'pyromark_black'
        return 'al2024_anodized'
    if node == 'polySurfac' and material == 'blinn3SG.001':
        if normal[2] > 0.4 and center[1] > 3.4:
            return 'window_quartz'
        return 'al5056_anodized'
    return BY_MATERIAL.get(material, 'al2024_anodized')


def read_glb(path):
    data = open(path, 'rb').read()
    length = struct.unpack('<I', data[12:16])[0]
    gltf = json.loads(data[20:20 + length])
    at = 20 + length
    binary = data[at + 8:at + 8 + struct.unpack('<I', data[at:at + 4])[0]]
    return gltf, binary


def creased_normals(points, faces):
    """Per-corner normals: summed by area from the faces at the same place within the crease."""
    a, b, c = points[faces[:, 0]], points[faces[:, 1]], points[faces[:, 2]]
    cross = np.cross(b - a, c - a)
    area = np.linalg.norm(cross, axis=1)
    unit = np.divide(cross, area[:, None], out=np.zeros_like(cross), where=area[:, None] > 0)
    span = float(np.ptp(points, axis=0).max()) or 1.0
    keys = np.round(points / (span * 1e-6)).astype(np.int64)
    corners = faces.reshape(-1)
    groups = {}
    for corner, vertex in enumerate(corners):
        groups.setdefault(tuple(keys[vertex]), []).append(corner)
    min_cos = np.cos(np.radians(CREASE_DEGREES))
    normals = np.zeros((len(corners), 3))
    for members in groups.values():
        idx = np.array(members)
        f = idx // 3
        n = unit[f]
        w = area[f]
        dots = n @ n.T
        mask = dots >= min_cos
        sums = (mask * w[None, :]) @ n
        length = np.linalg.norm(sums, axis=1)
        good = length > 0
        sums[good] /= length[good][:, None]
        sums[~good] = n[~good]
        normals[idx] = sums
    return normals


def main():
    source = sys.argv[1] if len(sys.argv) > 1 else 'scenes/models/apollo/apollo_lunar_module.glb'
    target = sys.argv[2] if len(sys.argv) > 2 else 'scenes/models/apollo/lm5_eagle.glb'
    gltf, binary = read_glb(source)
    materials = [m.get('name') for m in gltf['materials']]

    def view(index):
        v = gltf['bufferViews'][index]
        offset = v.get('byteOffset', 0)
        return binary[offset:offset + v['byteLength']]

    parts = {}       # name -> list of (points, faces)
    for node in gltf['nodes']:
        if 'mesh' not in node:
            continue
        if any(k in node for k in ('matrix', 'translation', 'rotation', 'scale')):
            sys.exit("a node has a transform, which this script doesn't apply")
        for primitive in gltf['meshes'][node['mesh']]['primitives']:
            mesh = DracoPy.decode(view(primitive['extensions']['KHR_draco_mesh_compression']['bufferView']))
            points = np.array(mesh.points, dtype=np.float64).reshape(-1, 3)
            faces = np.array(mesh.faces, dtype=np.int64).reshape(-1, 3)
            given = getattr(mesh, 'normals', None)
            a, b, c = points[faces[:, 0]], points[faces[:, 1]], points[faces[:, 2]]
            normal = np.cross(b - a, c - a)
            if given is not None and len(given) == len(points):
                # Some of NASA's faces are wound against their own normals: turn
                # those round, so a face's front (and the normals worked out
                # below) are the side the model's normals say is out.
                given = np.array(given, dtype=np.float64).reshape(-1, 3)
                out = given[faces[:, 0]] + given[faces[:, 1]] + given[faces[:, 2]]
                flip = np.einsum('ij,ij->i', normal, out) < 0
                faces[flip] = faces[flip][:, [0, 2, 1]]
                a, b, c = points[faces[:, 0]], points[faces[:, 1]], points[faces[:, 2]]
                normal = np.cross(b - a, c - a)
            length = np.linalg.norm(normal, axis=1)
            keep = length > 0
            normal[keep] /= length[keep][:, None]
            center = (a + b + c) / 3.0
            material = materials[primitive['material']]
            names = [face_part(material, node.get('name'), normal[i], center[i]) for i in range(len(faces))]
            for name in set(names):
                chosen = faces[[i for i in range(len(faces)) if names[i] == name and keep[i]]]
                if len(chosen):
                    parts.setdefault(name, []).append((points, chosen))

    # Place: scale to meters, stand on y = 0, centered in x and z.
    every = np.concatenate([p for name in parts for p, _ in parts[name]])
    low, high = every.min(axis=0), every.max(axis=0)
    shift = np.array([-(low[0] + high[0]) / 2.0, -low[1], -(low[2] + high[2]) / 2.0])

    blob = bytearray()
    accessors, views, primitives, out_materials = [], [], [], []

    def add_view(raw, target_kind):
        while len(blob) % 4:
            blob.append(0)
        views.append({'buffer': 0, 'byteOffset': len(blob), 'byteLength': len(raw), 'target': target_kind})
        blob.extend(raw)
        return len(views) - 1

    for name in sorted(parts):
        points_list, faces_list, base = [], [], 0
        for points, faces in parts[name]:
            used = np.unique(faces)
            remap = -np.ones(len(points), dtype=np.int64)
            remap[used] = np.arange(len(used)) + base
            points_list.append((points[used] + shift) * SCALE)
            faces_list.append(remap[faces])
            base += len(used)
        points = np.concatenate(points_list)
        faces = np.concatenate(faces_list)
        normals = creased_normals(points, faces)
        # One vertex per distinct (position, normal) pair.
        corner_points = points[faces.reshape(-1)]
        stacked = np.round(np.hstack([corner_points, normals]), 7).astype(np.float32)
        unique, inverse = np.unique(stacked, axis=0, return_inverse=True)
        positions = unique[:, :3].astype(np.float32)
        vertex_normals = unique[:, 3:].astype(np.float32)
        vertex_normals /= np.maximum(np.linalg.norm(vertex_normals, axis=1), 1e-12)[:, None]
        indices = inverse.reshape(-1).astype(np.uint32)

        pv = add_view(positions.tobytes(), 34962)
        accessors.append({'bufferView': pv, 'componentType': 5126, 'count': len(positions), 'type': 'VEC3',
                          'min': positions.min(axis=0).tolist(), 'max': positions.max(axis=0).tolist()})
        nv = add_view(vertex_normals.astype(np.float32).tobytes(), 34962)
        accessors.append({'bufferView': nv, 'componentType': 5126, 'count': len(vertex_normals), 'type': 'VEC3'})
        iv = add_view(indices.tobytes(), 34963)
        accessors.append({'bufferView': iv, 'componentType': 5125, 'count': len(indices), 'type': 'SCALAR'})
        out_materials.append({'name': name, 'pbrMetallicRoughness': {
            'baseColorFactor': list(PART_COLORS[name]) + [1.0], 'metallicFactor': 0.0, 'roughnessFactor': 0.6}})
        primitives.append({'attributes': {'POSITION': len(accessors) - 3, 'NORMAL': len(accessors) - 2},
                           'indices': len(accessors) - 1, 'material': len(out_materials) - 1, 'mode': 4})
        print("%-16s %7d faces %7d vertices" % (name, len(faces), len(positions)))

    while len(blob) % 4:
        blob.append(0)
    out = {
        'asset': {'version': '2.0', 'generator': 'Ray tools/make_lm5_eagle.py',
                  'copyright': 'NASA (public domain), parts named and normals worked out by Ray'},
        'scene': 0, 'scenes': [{'nodes': [0]}],
        'nodes': [{'name': 'lm5_eagle', 'mesh': 0}],
        'meshes': [{'name': 'lm5_eagle', 'primitives': primitives}],
        'materials': out_materials, 'accessors': accessors, 'bufferViews': views,
        'buffers': [{'byteLength': len(blob)}],
    }
    text = json.dumps(out, separators=(',', ':')).encode('utf-8')
    while len(text) % 4:
        text += b' '
    with open(target, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, 12 + 8 + len(text) + 8 + len(blob)))
        f.write(struct.pack('<II', len(text), 0x4E4F534A))
        f.write(text)
        f.write(struct.pack('<II', len(blob), 0x004E4942))
        f.write(blob)
    print("wrote", target)


if __name__ == '__main__':
    main()
