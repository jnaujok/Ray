#!/usr/bin/env python3
"""Write the sample scenes for implicit surfaces, blobs and fractals.

    tools/make_math_scenes.py

Run from the repository root. It writes scenes/implicit_test.ray,
scenes/blob_test.ray and scenes/fractal_test.ray from shapes_test.ray's
settings (camera, lights, floor). Re-save each with
`raythrower scenes/<scene>.ray --save-json scenes/<scene>.ray` afterwards so
it is written the way Ray writes scenes.
"""
import json

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


def scene(name, objects, cam, size=None):
    s = json.loads(json.dumps({k: v for k, v in base.items() if k != 'objects'}))
    s['settings']['imageFile'] = name + '.out'
    if size:
        s['settings']['imageWidth'], s['settings']['imageHeight'] = size
    s['camera'].update(cam)
    s['objects'] = [floor] + objects
    with open('scenes/' + name + '.ray', 'w') as f:
        json.dump(s, f, indent=1)


# A gyroid's labyrinth (one side of it), kept inside a ball by a csg.
GYROID = {"type": "implicit", "center": [0, 0, 0],
          "formula": "sin(k*x)*cos(k*y) + sin(k*y)*cos(k*z) + sin(k*z)*cos(k*x) + 0.3",
          "parameters": {"k": 5.0}, "bounds": 1.0}

# Taubin's heart, turned so its point is down and +y is up.
HEART = "(x^2 + 2.25*z^2 + y^2 - 1)^3 - x^2*y^3 - 0.1125*z^2*y^3"

scene('implicit_test', [
    {"type": "implicit", "center": [-3.1, 0.82, 0.6], "formula": "x^2 + y^2 + z^2 - 1", "bounds": 1.0, "scale": 0.8,
     "color": [0.5, 0.5, 0.5], "materials": mats([0.85, 0.25, 0.2])},
    {"type": "sphere", "center": [-1.3, 0.82, 0.6], "radius": 0.8,
     "color": [0.5, 0.5, 0.5], "materials": mats([0.85, 0.25, 0.2])},
    {"type": "csg", "operation": "intersection", "objects": [
        GYROID, {"type": "sphere", "center": [0, 0, 0], "radius": 0.95}],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.95, 0.75, 0.25], spec=0.4)},
    {"type": "implicit", "center": [2.9, 1.15, 0.8], "formula": HEART, "bounds": [1.2, 1.3, 0.8], "scale": 0.9,
     "rotation": [0, 0, 0], "color": [0.5, 0.5, 0.5], "materials": mats([0.9, 0.1, 0.25], spec=0.5)},
], {})

# The csg's parts are worked out where it is: move the gyroid ball into place.
s = json.load(open('scenes/implicit_test.ray'))
csg = s['objects'][3]
for part in csg['objects']:
    part['center'] = [0.85, 1.0, 0.4]
json.dump(s, open('scenes/implicit_test.ray', 'w'), indent=1)

scene('blob_test', [
    {"type": "blob", "center": [-1.6, 0.0, 0.5], "balls": [
        {"center": [0.0, 0.9, 0.0], "radius": 1.0},
        {"center": [0.75, 1.0, 0.1], "radius": 0.85},
        {"center": [0.3, 1.6, -0.2], "radius": 0.7},
        {"center": [-0.6, 0.7, 0.2], "radius": 0.8},
        {"center": [1.3, 0.7, -0.1], "radius": 0.6}],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.2, 0.55, 0.9], spec=0.5, refl=0.1)},
    {"type": "blob", "center": [2.0, 0.0, 0.6], "balls": [
        {"center": [0.0, 0.95, 0.0], "radius": 2.0},
        {"center": [-0.25, 1.2, -0.95], "radius": 0.75, "strength": -2.5},
        {"center": [0.45, 0.65, -0.85], "radius": 0.5, "strength": -2.0}],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.95, 0.6, 0.2], spec=0.5)},
], {})

scene('fractal_test', [
    {"type": "fractal", "kind": "mandelbulb", "center": [-2.7, 1.15, 0.6], "scale": 1.0, "rotation": [0, 30, 0],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.8, 0.55, 0.35], spec=0.3)},
    {"type": "fractal", "kind": "menger", "center": [0.0, 0.8, 0.6], "scale": 0.8, "rotation": [0, 30, 0],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.35, 0.6, 0.85], spec=0.3)},
    {"type": "fractal", "kind": "sierpinski", "center": [2.7, 0.53, 0.6], "scale": 0.9, "rotation": [-45, 0, 35.264],
     "color": [0.5, 0.5, 0.5], "materials": mats([0.85, 0.85, 0.85], spec=0.3)},
], {})
