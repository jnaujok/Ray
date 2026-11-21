#!/usr/bin/env python3
"""Writes the Apollo camera test scenes (2026-alpha-0.20).

    make_apollo_scenes.py [scenes directory]

  apollo_lenses_test.ray   the Hasselblad Electric Camera (80 mm Planar,
                           Ektachrome MS) on a sunlit test subject
  film_test.ray            gray steps, colors and a glowing lamp through
                           Kodak film: the curve, grain and halation
  reseau_test.ray          the Data Camera's Reseau crosses, and a bright
                           sphere blooming over one of them
  tranquility_test.ray     Tranquility Base: a lunar module shape on
                           regolith in low sun, shot with the Data Camera

The scenes are in meters. Only the Python standard library is needed.
"""
import json
import math
import os
import sys


def settings(name, width, height, depth=8):
    return {"imageWidth": width, "imageHeight": height, "imageFile": name + ".out",
            "colorDepth": 8, "maximumDepth": depth, "depthThreshold": 0.01}


def mats(*items):
    return list(items)


def ambient(c):
    return {"type": "ambient", "influence": 1.0, "color": [c, c, c] if isinstance(c, (int, float)) else c}


def diffuse(c, influence=0.9, texture=None):
    m = {"type": "diffuse", "influence": influence, "color": c, "diffuseCast": False}
    if texture:
        m["texture"] = texture
    return m


def specular(power=0.25, influence=0.3):
    return {"type": "specular", "influence": influence, "color": [1.0, 1.0, 1.0], "power": power,
            "indexOfRefraction": 1.5, "materialHighlight": False}


def reflective(c, influence=0.5, roughness=0.0):
    m = {"type": "reflective", "influence": influence, "color": c, "diffuseReflectivity": 0.0, "filterReflection": False}
    if roughness:
        m["roughness"] = roughness
    return m


def regolith(albedo=0.12):
    return {"type": "regolith", "influence": 1.0, "color": [albedo, albedo * 0.97, albedo * 0.93],
            "surge": 1.0, "width": 0.06}


def sun(direction, color, angle=0.53, samples=4):
    n = math.sqrt(sum(d * d for d in direction))
    return {"type": "directional", "direction": [round(d / n, 6) for d in direction], "color": color,
            "angle": angle, "samples": samples}


def plane(y, materials, bump=None):
    o = {"type": "plane", "position": [0.0, y, 0.0], "normal": [0.0, 1.0, 0.0], "color": [0.5, 0.5, 0.5],
         "materials": materials}
    if bump:
        o["bump"] = bump
    return o


def sphere(center, radius, materials, bump=None):
    o = {"type": "sphere", "center": center, "radius": radius, "color": [0.5, 0.5, 0.5], "materials": materials}
    if bump:
        o["bump"] = bump
    return o


def box(center, size, materials, rotation=(0.0, 0.0, 0.0), bump=None):
    o = {"type": "box", "center": center, "size": size, "rotation": list(rotation), "color": [0.5, 0.5, 0.5],
         "materials": materials}
    if bump:
        o["bump"] = bump
    return o


def cylinder(base, top, radius, materials, capped=True):
    return {"type": "cylinder", "base": base, "top": top, "radius": radius, "capped": capped,
            "color": [0.5, 0.5, 0.5], "materials": materials}


def scene(name, width, height, camera, objects, lights, background=(0.0, 0.0, 0.0), depth=8):
    return {"format": "Ray scene", "version": 3, "settings": settings(name, width, height, depth),
            "background": list(background), "camera": camera, "objects": objects, "lights": lights}


REGOLITH_BUMP = {"mode": "height", "texture": {"type": "noise", "scale": 0.35, "octaves": 5, "turbulence": 0.0,
                                               "seed": 11},
                 "depth": 0.015, "step": 0.002}


def apollo_lenses():
    camera = {"position": [0.0, 1.4, -6.0], "lookAt": [0.0, 0.8, 0.0], "up": [0.0, 1.0, 0.0],
              "body": "hasselblad-hec",
              "sampler": {"type": "stratified", "grid": 3, "seed": 0},
              "lens": {"focusDistance": 6.0, "millimetersPerUnit": 1000.0, "fNumber": 4.0}}
    gray = [0.18, 0.18, 0.18]
    objects = [
        plane(0.0, mats(ambient(0.02), diffuse([0.7, 0.7, 0.68], 0.9,
                                               {"type": "checker", "colors": [[0.25, 0.25, 0.25], [0.75, 0.75, 0.72]],
                                                "size": 0.5, "solid": False}))),
        box([0.0, 1.0, 1.5], [2.6, 2.0, 0.05], mats(ambient(0.02), diffuse([1.0, 1.0, 1.0], 0.9,
            {"type": "checker", "colors": [[0.05, 0.05, 0.05], [0.9, 0.9, 0.9]], "size": 0.2, "solid": False}))),
        sphere([-0.8, 0.35, 0.0], 0.35, mats(ambient(0.02), diffuse([0.8, 0.2, 0.15]), specular())),
        sphere([0.0, 0.35, -0.6], 0.35, mats(ambient(0.02), diffuse(gray), specular())),
        sphere([0.8, 0.35, 0.0], 0.35, mats(ambient(0.02), diffuse([0.15, 0.3, 0.8]), specular())),
    ]
    lights = [sun([0.4, -0.6, 0.7], [1.6, 1.6, 1.55])]
    return scene("apollo_lenses_test", 160, 160, camera, objects, lights, (0.02, 0.02, 0.03))


def film():
    camera = {"position": [0.0, 1.0, -5.0], "lookAt": [0.0, 0.7, 0.0], "up": [0.0, 1.0, 0.0],
              "verticalAngle": 34.0, "horizontalAngle": 44.2, "aspectRatio": 1.333333,
              "sampler": {"type": "stratified", "grid": 2, "seed": 0},
              "colorModel": {"type": "film", "stock": "ektachrome-ms", "shutter": 0.004, "fStop": 11.0,
                             "width": 20.0, "grain": 1.0}}
    objects = [plane(0.0, mats(ambient(0.01), diffuse([0.3, 0.3, 0.3])))]
    # Gray steps, one stop apart, from 0.02 to 1.28.
    for i in range(7):
        g = 0.02 * (2 ** i)
        objects.append(box([-1.8 + i * 0.6, 0.25, 0.0], [0.55, 0.5, 0.3], mats(diffuse([g, g, g], 1.0))))
    objects.append(sphere([-1.0, 1.0, 0.4], 0.3, mats(diffuse([0.8, 0.15, 0.1]), specular())))
    objects.append(sphere([0.0, 1.0, 0.4], 0.3, mats(diffuse([0.2, 0.7, 0.2]), specular())))
    objects.append(sphere([1.0, 1.0, 0.4], 0.3, mats(diffuse([0.15, 0.25, 0.85]), specular())))
    # A glowing lamp, far overexposed, to show halation.
    objects.append(sphere([1.9, 1.5, 0.6], 0.06, mats(ambient([60.0, 50.0, 35.0]))))
    lights = [sun([0.3, -0.7, 0.6], [2.2, 2.2, 2.1], 0.53, 2)]
    return scene("film_test", 240, 180, camera, objects, lights, (0.0, 0.0, 0.0))


def reseau():
    # The crosses are drawn wider than the plate's own (0.05 mm) so they
    # show at this small size.
    camera = {"position": [0.0, 1.0, -7.0], "lookAt": [0.0, 1.0, 0.0], "up": [0.0, 1.0, 0.0],
              "body": "hasselblad-hdc",
              "sampler": {"type": "stratified", "grid": 3, "seed": 0},
              "lens": {"focusDistance": 7.0, "millimetersPerUnit": 1000.0},
              "reseau": {"count": 5, "spacing": 10.0, "size": 2.0, "lineWidth": 0.12},
              "colorModel": {"type": "film", "stock": "ektachrome-ms", "shutter": 0.004, "fStop": 11.0,
                             "width": 53.0, "spread": 2.5}}
    objects = [
        plane(0.0, mats(regolith(0.12)), REGOLITH_BUMP),
        # A sunlit white sphere: far overexposed, it blooms over the cross
        # on its limb.
        sphere([1.15, 1.6, 0.0], 0.75, mats(ambient(8.0), diffuse([0.95, 0.95, 0.95], 1.0))),
        # A gray one that stays on the curve, under another cross.
        sphere([-1.15, 1.15, 0.0], 0.55, mats(diffuse([0.12, 0.12, 0.12], 1.0))),
    ]
    lights = [sun([-0.5, -0.45, 0.75], [5.0, 5.0, 4.9])]
    return scene("reseau_test", 320, 320, camera, objects, lights, (0.0, 0.0, 0.0))


def lunar_module():
    """A lunar module built from primitives (no NASA model): descent
    stage in gold foil on four legs, the ascent stage above it."""
    gold = mats(ambient([0.03, 0.025, 0.01]), diffuse([0.55, 0.42, 0.12], 0.5),
                reflective([0.95, 0.75, 0.3], 0.45, 0.35))
    foil_bump = {"mode": "height", "texture": {"type": "noise", "scale": 0.08, "octaves": 3, "turbulence": 1.0,
                                               "seed": 3}, "depth": 0.01, "step": 0.002}
    gray = mats(ambient(0.02), diffuse([0.62, 0.62, 0.6], 0.8), specular(0.3, 0.2))
    dark = mats(ambient(0.01), diffuse([0.08, 0.08, 0.08], 0.9))
    silver = mats(ambient(0.02), diffuse([0.5, 0.5, 0.5], 0.4), reflective([0.8, 0.8, 0.8], 0.5, 0.2))
    objs = [box([0.0, 1.55, 0.0], [3.2, 1.6, 3.2], gold, (0.0, 45.0, 0.0), foil_bump)]
    # The ascent stage: a rounded box with a dark window.
    objs.append({"type": "roundedBox", "center": [0.0, 3.25, 0.0], "size": [2.6, 1.9, 2.4], "radius": 0.35,
                 "rotation": [0.0, 45.0, 0.0], "color": [0.5, 0.5, 0.5], "materials": gray})
    objs.append(box([-0.62, 3.45, -0.62], [0.55, 0.45, 0.05], dark, (0.0, 45.0, 0.0)))
    # Legs and footpads.
    for k in range(4):
        a = math.radians(45.0 + 90.0 * k + 45.0)
        dx, dz = math.cos(a), math.sin(a)
        objs.append(cylinder([dx * 1.4, 1.2, dz * 1.4], [dx * 3.6, 0.18, dz * 3.6], 0.08, silver))
        objs.append(cylinder([dx * 3.6, 0.0, dz * 3.6], [dx * 3.6, 0.12, dz * 3.6], 0.42, silver))
    return objs


def tranquility():
    # The Data Camera standing west of the LM, looking east down-sun with
    # the sun low behind and to the left, in the manner of the Apollo 11
    # pictures of the LM (an imitation, not a recovered camera station).
    camera = {"position": [-9.5, 1.35, -7.5], "lookAt": [0.0, 1.6, 0.0], "up": [0.0, 1.0, 0.0],
              "body": "hasselblad-hdc",
              "sampler": {"type": "stratified", "grid": 3, "seed": 0},
              "lens": {"focusDistance": 12.0, "millimetersPerUnit": 1000.0}}
    crater = {"mode": "height", "texture": {"type": "noise", "scale": 2.5, "octaves": 6, "turbulence": 0.0,
                                            "seed": 7},
              "depth": 0.06, "step": 0.01}
    objects = [plane(0.0, mats(regolith(0.11)), crater)]
    objects += lunar_module()
    # The flag: a pole and a stiffened cloth.
    objects.append(cylinder([-3.5, 0.0, 3.0], [-3.5, 2.5, 3.0], 0.02,
                            mats(ambient(0.02), diffuse([0.7, 0.7, 0.7], 0.7), specular())))
    objects.append(box([-3.0, 2.15, 3.0], [0.95, 0.62, 0.01], mats(ambient(0.02), diffuse([1.0, 1.0, 1.0], 0.9,
        {"type": "checker", "colors": [[0.8, 0.1, 0.12], [0.92, 0.92, 0.9]], "size": 0.09, "solid": False}))))
    # Some rocks.
    for i, (x, z, r) in enumerate([(-6.0, -2.5, 0.18), (-4.2, -4.8, 0.12), (2.5, -5.0, 0.25), (-7.5, 0.5, 0.1)]):
        objects.append(sphere([x, r * 0.6, z], r, mats(regolith(0.13)), REGOLITH_BUMP))
    lights = [sun([0.75, -0.26, 0.6], [2.6, 2.6, 2.55])]
    return scene("tranquility_test", 192, 192, camera, objects, lights, (0.0, 0.0, 0.0), 6)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "scenes")
    for name, fn in (("apollo_lenses_test", apollo_lenses), ("film_test", film), ("reseau_test", reseau),
                     ("tranquility_test", tranquility)):
        with open(os.path.join(out, name + ".ray"), "w") as f:
            json.dump(fn(), f, indent=2)
            f.write("\n")
        print("wrote", name)


if __name__ == "__main__":
    main()
