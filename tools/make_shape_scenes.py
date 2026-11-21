#!/usr/bin/env python3
"""Write the sample scenes for lathes, extrusions and sweeps.

    tools/make_shape_scenes.py

Run from the repository root. It writes scenes/lathe_test.ray and
scenes/extrusion_test.ray from shapes_test.ray's settings (camera, lights,
floor), with the profiles, outlines and paths worked out here (a star's
points, a helix, a figure-eight loop). Re-save each with
`raythrower scenes/<scene>.ray --save-json scenes/<scene>.ray` afterwards so
it is written the way Ray writes scenes.
"""
import json
import math

base = json.load(open('scenes/shapes_test.ray'))
floor = base['objects'][0]


def mats(c, spec=0.3, refl=0.0, amb=0.1):
    m = [{"type": "ambient", "influence": 1.0, "color": [c[0] * amb, c[1] * amb, c[2] * amb]},
         {"type": "diffuse", "influence": 0.85, "color": c}]
    if spec:
        m.append({"type": "specular", "influence": spec, "color": [1, 1, 1], "power": 0.2, "indexOfRefraction": 1.5})
    if refl:
        m.append({"type": "reflective", "influence": refl, "color": [1, 1, 1]})
    return m


GLASS = [{"type": "reflective", "influence": 0.08, "color": [1, 1, 1]},
         {"type": "transparent", "influence": 0.92, "color": [1, 1, 1], "indexOfRefraction": 1.5,
          "attenuationColor": [0.75, 0.9, 1.0], "attenuationDistance": 1.0},
         {"type": "specular", "influence": 0.4, "color": [1, 1, 1], "power": 0.1, "indexOfRefraction": 1.5}]


def scene(name, objects, cam):
    s = json.loads(json.dumps({k: v for k, v in base.items() if k != 'objects'}))
    s['settings']['imageFile'] = name + '.out'
    s['camera'].update(cam)
    for o in objects:
        # Lifted off the floor a hair so their bases don't fight with it.
        for key in ('center', 'position'):
            if key in o:
                o[key] = [o[key][0], round(o[key][1] + 0.002, 6), o[key][2]]
    s['objects'] = [floor] + objects
    with open('scenes/' + name + '.ray', 'w') as f:
        json.dump(s, f, indent=1)


# Lathe profiles: [r, y] points and curves.
PAWN = [[0, 0], [0.45, 0], [0.45, 0.08], {"quadratic": [[0.45, 0.16], [0.32, 0.18]]}, [0.3, 0.22],
        {"cubic": [[0.18, 0.3], [0.14, 0.5], [0.13, 0.62]]}, [0.25, 0.64], [0.25, 0.69], [0.13, 0.72],
        {"cubic": [[0.27, 0.74], [0.27, 1.02], [0, 1.02]]}]
ROOK = [[0, 0], [0.5, 0], [0.5, 0.1], [0.4, 0.16], {"cubic": [[0.3, 0.4], [0.28, 0.7], [0.3, 0.9]]},
        [0.4, 0.95], [0.4, 1.25], [0.3, 1.25], [0.3, 1.13], [0, 1.13]]
QUEEN = [[0, 0], [0.5, 0.0], [0.5, 0.07], [0.4, 0.14], [0.3, 0.3], [0.2, 0.7], [0.16, 1.1], [0.3, 1.18],
         [0.18, 1.26], [0.24, 1.42], [0.14, 1.52], [0.09, 1.62], [0, 1.68]]
GOBLET = [[0, 0], [0.5, 0], [0.5, 0.03], {"quadratic": [[0.1, 0.05], [0.06, 0.12]]}, [0.05, 0.62],
          {"cubic": [[0.5, 0.66], [0.55, 1.1], [0.46, 1.45]]}, [0.43, 1.45],
          {"cubic": [[0.5, 1.12], [0.42, 0.74], [0, 0.72]]}]
VASE = [[0, 0], [0.42, 0], {"cubic": [[0.8, 0.5], [0.1, 0.9], [0.17, 1.3]]}, {"quadratic": [[0.2, 1.45], [0.3, 1.5]]}]
ORANGE = [[0, 0], {"cubic": [[0.36, 0], [0.62, 0.22], [0.62, 0.5]]}, {"cubic": [[0.62, 0.78], [0.36, 1.0], [0, 1.0]]}]

LATHE = [
    {"type": "lathe", "center": [-3.0, 0, 0.6], "profile": PAWN, "color": [0.5, 0.5, 0.5],
     "materials": mats([0.15, 0.12, 0.1], 0.5, 0.1)},
    {"type": "lathe", "center": [-1.8, 0, 0.9], "profile": ROOK, "color": [0.5, 0.5, 0.5],
     "materials": mats([0.9, 0.85, 0.75], 0.4)},
    {"type": "lathe", "center": [-0.6, 0, 1.3], "profile": QUEEN, "smooth": True, "color": [0.5, 0.5, 0.5],
     "materials": mats([0.15, 0.12, 0.1], 0.5, 0.1)},
    {"type": "lathe", "center": [0.7, 0, 0.2], "profile": GOBLET, "color": [0.5, 0.5, 0.5], "materials": GLASS},
    {"type": "lathe", "center": [2.0, 0, 1.0], "profile": VASE, "color": [0.5, 0.5, 0.5],
     "materials": [{"type": "ambient", "influence": 1.0, "color": [0.08, 0.05, 0.03]},
                   {"type": "diffuse", "influence": 0.85, "color": [1, 1, 1], "texture": {"preset": "terracotta"}}]},
    {"type": "lathe", "center": [2.8, 0, -0.5], "profile": ORANGE, "sweep": 270, "rotation": [0, 63, 0],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.95, 0.5, 0.08], 0.3)},
]

# Extrusion outlines and sweep paths.
STAR = []
for k in range(10):
    a = math.pi / 2 + k * math.pi / 5
    r = 0.7 if k % 2 == 0 else 0.3
    STAR.append([round(r * math.cos(a), 6), round(r * math.sin(a), 6)])
SQUARE = [[-0.35, -0.35], [0.35, -0.35], [0.35, 0.35], [-0.35, 0.35]]
K = 0.5523


def circle(cx, cy, r):
    return [[cx + r, cy], {"cubic": [[cx + r, cy + K * r], [cx + K * r, cy + r], [cx, cy + r]]},
            {"cubic": [[cx - K * r, cy + r], [cx - r, cy + K * r], [cx - r, cy]]},
            {"cubic": [[cx - r, cy - K * r], [cx - K * r, cy - r], [cx, cy - r]]},
            {"cubic": [[cx + K * r, cy - r], [cx + r, cy - K * r], [cx + r, cy]]}]


FRAME = [[-0.4, -0.55], [0.4, -0.55], {"quadratic": [[0.55, -0.55], [0.55, -0.4]]}, [0.55, 0.4],
         {"quadratic": [[0.55, 0.55], [0.4, 0.55]]}, [-0.4, 0.55], {"quadratic": [[-0.55, 0.55], [-0.55, 0.4]]},
         [-0.55, -0.4], {"quadratic": [[-0.55, -0.55], [-0.4, -0.55]]}]
HELIX = []
for i in range(0, 4 * 12 + 1):
    a = i * math.pi / 6
    HELIX.append([round(1.0 + 0.38 * math.cos(a), 6), round(0.06 + 1.1 * i / 48, 6), round(1.2 + 0.38 * math.sin(a), 6)])
KNOT = []
for i in range(16):
    a = 2 * math.pi * i / 16
    KNOT.append([round(2.6 + 0.9 * math.sin(a), 6), round(0.12 + 0.06 * math.cos(a), 6), round(0.45 * math.sin(2 * a) - 0.6, 6)])
ZIGZAG = [[-3.4, 0.08, -1.2], [-2.6, 0.5, -1.0], [-1.8, 0.08, -0.8], [-1.0, 0.5, -0.6], [-0.4, 0.08, -0.6]]

EXTRUSION = [
    {"type": "extrusion", "outline": STAR, "depth": 1.3, "taper": 0.25, "position": [-2.6, 0, 0.9], "rotation": [-90, 0, 0],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.95, 0.75, 0.2], 0.5, 0.15)},
    {"type": "extrusion", "outline": SQUARE, "depth": 1.7, "twist": 120, "position": [-1.1, 0, 1.3], "rotation": [-90, 0, 0],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.3, 0.55, 0.9], 0.4)},
    {"type": "extrusion", "outlines": [FRAME, circle(0, 0, 0.3)], "depth": 0.3, "bevel": 0.06, "backBevel": 0.06,
     "position": [0.3, 0.56, 0.4], "rotation": [0, -20, 0], "color": [0.5, 0.5, 0.5], "materials": mats([0.85, 0.2, 0.25], 0.5)},
    {"type": "sweep", "path": HELIX, "radius": 0.05, "color": [0.5, 0.5, 0.5], "materials": mats([0.8, 0.8, 0.85], 0.6, 0.35)},
    {"type": "sweep", "path": KNOT, "closed": True, "radius": 0.1, "color": [0.5, 0.5, 0.5], "materials": mats([0.2, 0.75, 0.35], 0.4)},
    {"type": "sweep", "path": ZIGZAG, "smooth": False, "outline": [[-0.06, -0.06], [0.06, -0.06], [0.06, 0.06], [-0.06, 0.06]],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.6, 0.4, 0.25], 0.3)},
]

if __name__ == '__main__':
    scene('lathe_test', LATHE, {"position": [0.0, 2.8, -9.0], "lookAt": [0.0, 0.6, 0.4]})
    scene('extrusion_test', EXTRUSION, {"position": [0.0, 3.0, -9.0], "lookAt": [0.0, 0.5, 0.2]})
