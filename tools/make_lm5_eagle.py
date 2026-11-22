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
    rcs_stainless     the RCS thruster quads (polished 304 stainless steel)
    plume_deflector   the RCS plume deflectors below the quads

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
import math
import os
import random
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
    'rcs_stainless': (0.55, 0.55, 0.56), 'plume_deflector': (0.13, 0.13, 0.135),
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
for _node in ('10', '11', '12', '13', '14', 'g1', 'g2', 'g3', 'g4', 'g5', 'g6', 'g7', 'g8', 'g9', 'gr', 'p1'):
    BY_NODE[('blinn1SG.002', 'group13 ' + _node)] = 'rcs_stainless'      # the RCS thruster quads
for _node in ('10', '11', 'p9', 'pP'):
    BY_NODE[('blinn6SG.001', 'group75 ' + _node)] = 'plume_deflector'    # the panels under the RCS quads
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


# The landing crushed about 15 cm of the primary struts' honeycomb: everything
# above the footpads comes down by that much, the struts' lower sections
# shortening smoothly between the pads' tops and CRUSH_TOP (meters, after
# scaling). The engine bell, which reaches into that band, moves down whole.
CRUSH = 0.15
PAD_TOP = 0.24
CRUSH_TOP = 1.25


def crush_drop(y):
    t = np.clip((y - PAD_TOP) / (CRUSH_TOP - PAD_TOP), 0.0, 1.0)
    return CRUSH * t * t * (3.0 - 2.0 * t)


def crush(points, name):
    points = points.copy()
    if name == 'dps_bell':
        points[:, 1] -= crush_drop(points[:, 1].max())
    else:
        points[:, 1] -= crush_drop(points[:, 1])
    return points


# The ascent stage's thin anodized panels: flat regions of faces (within
# PANEL_DEGREES of each other), each bellied out by tilting the normals at its
# edges PILLOW_DEGREES away from its middle, each with its own place in a
# texture atlas (lm5_eagle_panels.png: its color, a little different from its
# neighbors', with a seam and rivets around its edge; lm5_eagle_rivets.png:
# the rivets' heights, for a bump map), at TEXELS a meter.
PANEL_PARTS = {'al2024_anodized': '#A3A6A6', 'al5056_anodized': '#CBBF9C', 'al_olive': '#7A7966'}
PANEL_DEGREES = 3.0
PILLOW_DEGREES = 5.0
MIN_PANEL_AREA = 0.03
TEXELS = 160.0
ATLAS = 2048
PAD = 2
RIVET_SPACING = 0.07
RIVET_INSET = 0.022
RIVET_RADIUS = 0.007


def panel_jobs(name, points, faces, normals):
    """Splits a part into flat panels; bellies the big ones' normals; returns one job a panel for the atlas."""
    a, b, c = points[faces[:, 0]], points[faces[:, 1]], points[faces[:, 2]]
    cross = np.cross(b - a, c - a)
    area = np.linalg.norm(cross, axis=1) / 2.0
    unit = np.divide(cross, 2.0 * area[:, None], out=np.zeros_like(cross), where=area[:, None] > 0)
    span = float(np.ptp(points, axis=0).max()) or 1.0
    keys = np.round(points / (span * 1e-6)).astype(np.int64)
    _, weld = np.unique(keys, axis=0, return_inverse=True)
    w = weld.reshape(-1)[faces]
    edges = {}
    for f in range(len(faces)):
        for k in range(3):
            e = (min(w[f, k], w[f, (k + 1) % 3]), max(w[f, k], w[f, (k + 1) % 3]))
            edges.setdefault(e, []).append(f)
    adjacent = [[] for _ in range(len(faces))]
    for members in edges.values():
        for f in members:
            adjacent[f].extend(g for g in members if g != f)
    min_cos = math.cos(math.radians(PANEL_DEGREES))
    panel = -np.ones(len(faces), dtype=np.int64)
    jobs = []
    for seed in np.argsort(-area):
        if panel[seed] >= 0:
            continue
        panel[seed] = len(jobs)
        members, stack = [seed], [seed]
        while stack:
            g = stack.pop()
            for h in adjacent[g]:
                if panel[h] < 0 and unit[h] @ unit[seed] >= min_cos:
                    panel[h] = len(jobs)
                    members.append(h)
                    stack.append(h)
        members = np.array(members)
        total = area[members].sum()
        normal = (unit[members] * area[members, None]).sum(axis=0)
        normal = normal / (np.linalg.norm(normal) or 1.0) if total > 0 else unit[seed]
        middle = (((a + b + c) / 3.0)[members] * area[members, None]).sum(axis=0) / total if total > 0 else a[seed]
        count = {}
        for f in members:
            for k in range(3):
                e = (min(w[f, k], w[f, (k + 1) % 3]), max(w[f, k], w[f, (k + 1) % 3]))
                count[e] = count.get(e, 0) + 1
        boundary_edges = [e for e, n in count.items() if n == 1]
        on_edge = {v for e in boundary_edges for v in e}
        axis_u = np.cross(normal, [0.0, 1.0, 0.0])
        if np.linalg.norm(axis_u) < 0.1:
            axis_u = np.cross(normal, [1.0, 0.0, 0.0])
        axis_u /= np.linalg.norm(axis_u)
        axis_v = np.cross(normal, axis_u)
        corners = (members[:, None] * 3 + np.arange(3)[None, :]).reshape(-1)
        offsets = points[faces.reshape(-1)[corners]] - middle
        flat = offsets - np.outer(offsets @ normal, normal)
        radius = np.linalg.norm(flat, axis=1)
        if total >= MIN_PANEL_AREA:
            most = radius.max() or 1.0
            for i, corner in enumerate(corners):
                edge = w.reshape(-1)[corner] in on_edge
                tilt = math.radians(PILLOW_DEGREES) * (1.0 if edge else min(radius[i] / most, 1.0))
                out = flat[i] / radius[i] if radius[i] > 1e-9 else np.zeros(3)
                normals[corner] = math.cos(tilt) * normal + math.sin(tilt) * out
        uv = np.stack([offsets @ axis_u, offsets @ axis_v], axis=1)
        jobs.append({'name': name, 'corners': corners, 'uv': uv, 'big': total >= MIN_PANEL_AREA,
                     'faces': members, 'w': w, 'boundary': set(boundary_edges)})
    return jobs


def pack_and_draw(jobs, geometry, folder):
    """Packs the panels into the atlas (shelves, largest first), sets their UVs, draws both images."""
    from PIL import Image, ImageDraw
    texels = TEXELS
    while True:
        x = y = row = 0
        places = []
        for job in sorted(range(len(jobs)), key=lambda j: -np.ptp(jobs[j]['uv'][:, 1])):
            uv = jobs[job]['uv']
            width = int(math.ceil(np.ptp(uv[:, 0]) * texels)) + 2 * PAD + 1
            height = int(math.ceil(np.ptp(uv[:, 1]) * texels)) + 2 * PAD + 1
            if x + width > ATLAS:
                x, y, row = 0, y + row, 0
            places.append((job, x, y))
            x, row = x + width, max(row, height)
        if y + row <= ATLAS:
            break
        texels *= 0.9
    color = Image.new('RGB', (ATLAS, ATLAS), (128, 128, 128))
    height_map = Image.new('L', (ATLAS, ATLAS), 0)
    draw, bump = ImageDraw.Draw(color), ImageDraw.Draw(height_map)
    rng = random.Random(1969)
    for job, x0, y0 in places:
        info = jobs[job]
        uv = info['uv']
        low = uv.min(axis=0)
        pixels = (uv - low) * texels + np.array([x0 + PAD + 0.5, y0 + PAD + 0.5])
        points, faces, normals, uvs = geometry[info['name']]
        uvs[info['corners']] = pixels / ATLAS
        base = [int(info['name'] and PANEL_PARTS[info['name']][i:i + 2], 16) / 255.0 for i in (1, 3, 5)]
        shade = [ch * (1.0 + rng.gauss(0.0, 0.025)) for ch in base]
        if rng.random() < 0.3:            # some panels clearly bluer
            shade = [shade[0] * 0.93, shade[1] * 0.98, shade[2] * 1.07]
        rgb = tuple(int(round(min(max(ch, 0.0), 1.0) * 255)) for ch in shade)
        seam = tuple(int(ch * 0.72) for ch in rgb)
        rivet = tuple(int(ch * 0.82) for ch in rgb)
        triangles = pixels.reshape(-1, 3, 2)
        for tri in triangles:
            draw.polygon([tuple(p) for p in tri], fill=rgb, outline=rgb, width=2)
        if not info['big']:
            continue
        w = info['w']
        for i, f in enumerate(info['faces']):
            tri = triangles[i]
            for k in range(3):
                e = (min(w[f, k], w[f, (k + 1) % 3]), max(w[f, k], w[f, (k + 1) % 3]))
                if e not in info['boundary']:
                    continue
                p, q, r = tri[k], tri[(k + 1) % 3], tri[(k + 2) % 3]
                draw.line([tuple(p), tuple(q)], fill=seam, width=1)
                along = q - p
                length = np.linalg.norm(along) / texels
                if length < 2.0 * RIVET_INSET:
                    continue
                inward = np.array([-along[1], along[0]])
                inward /= np.linalg.norm(inward)
                if inward @ (r - p) < 0:
                    inward = -inward
                count = max(int(length / RIVET_SPACING), 1)
                size = max(RIVET_RADIUS * texels, 0.8)
                for n in range(count):
                    at = p + along * ((n + 0.5) / count) + inward * RIVET_INSET * texels
                    box = [at[0] - size, at[1] - size, at[0] + size, at[1] + size]
                    draw.ellipse(box, fill=rivet)
                    bump.ellipse(box, fill=255)
    color.save(os.path.join(folder, 'lm5_eagle_panels.png'), optimize=True)
    height_map.save(os.path.join(folder, 'lm5_eagle_rivets.png'), optimize=True)
    print("atlas: %d panels at %.0f texels a meter" % (len(jobs), texels))


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

    geometry, jobs = {}, []
    for name in sorted(parts):
        points_list, faces_list, base = [], [], 0
        for points, faces in parts[name]:
            used = np.unique(faces)
            remap = -np.ones(len(points), dtype=np.int64)
            remap[used] = np.arange(len(used)) + base
            points_list.append((points[used] + shift) * SCALE)
            faces_list.append(remap[faces])
            base += len(used)
        points = crush(np.concatenate(points_list), name)
        faces = np.concatenate(faces_list)
        normals = creased_normals(points, faces)
        uvs = np.zeros((len(normals), 2))
        if name in PANEL_PARTS:
            jobs.extend(panel_jobs(name, points, faces, normals))
        geometry[name] = (points, faces, normals, uvs)

    pack_and_draw(jobs, geometry, os.path.dirname(os.path.abspath(target)))

    for name in sorted(parts):
        points, faces, normals, uvs = geometry[name]
        # One vertex per distinct (position, normal, texture coordinates).
        corner_points = points[faces.reshape(-1)]
        stacked = np.round(np.hstack([corner_points, normals, uvs]), 7).astype(np.float32)
        unique, inverse = np.unique(stacked, axis=0, return_inverse=True)
        positions = unique[:, :3].astype(np.float32)
        vertex_normals = unique[:, 3:6].astype(np.float32)
        vertex_normals /= np.maximum(np.linalg.norm(vertex_normals, axis=1), 1e-12)[:, None]
        vertex_uvs = unique[:, 6:8].astype(np.float32)
        indices = inverse.reshape(-1).astype(np.uint32)

        pv = add_view(positions.tobytes(), 34962)
        accessors.append({'bufferView': pv, 'componentType': 5126, 'count': len(positions), 'type': 'VEC3',
                          'min': positions.min(axis=0).tolist(), 'max': positions.max(axis=0).tolist()})
        nv = add_view(vertex_normals.astype(np.float32).tobytes(), 34962)
        accessors.append({'bufferView': nv, 'componentType': 5126, 'count': len(vertex_normals), 'type': 'VEC3'})
        tv = add_view(vertex_uvs.tobytes(), 34962)
        accessors.append({'bufferView': tv, 'componentType': 5126, 'count': len(vertex_uvs), 'type': 'VEC2'})
        iv = add_view(indices.tobytes(), 34963)
        accessors.append({'bufferView': iv, 'componentType': 5125, 'count': len(indices), 'type': 'SCALAR'})
        out_materials.append({'name': name, 'pbrMetallicRoughness': {
            'baseColorFactor': list(PART_COLORS[name]) + [1.0], 'metallicFactor': 0.0, 'roughnessFactor': 0.6}})
        primitives.append({'attributes': {'POSITION': len(accessors) - 4, 'NORMAL': len(accessors) - 3,
                                          'TEXCOORD_0': len(accessors) - 2},
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
