#!/usr/bin/env python3
"""Write the LM-5 Eagle prefab and the AS11-40-5927 recreation.

    make_eagle_scenes.py

scenes/models/apollo/lm5_eagle.ray is the prefab: one mesh, lm5_eagle.glb (made
by tools/make_lm5_eagle.py), with a materialMap giving each named part the
materials the Apollo photographs call for. Any scene brings it in with
{"type": "include", "file": "models/apollo/lm5_eagle.ray", ...}.

scenes/as11_40_5927_test.ray recreates NASA photograph AS11-40-5927 (Apollo 11, GET
110:53:38, 1969-07-21 04:25:38 UTC): Eagle from the east, its aft
equipment bay to the right and its Scientific Equipment (SEQ) bay closed, under
a sun 14.96 degrees up at azimuth 90.2 (just south of east). Buzz Aldrin, who
stands at the bay in the photograph, is left out. The LM is turned so its
front (+Z, the hatch) faces azimuth 244 and sits 4 cm down (its footpads in the
regolith), and the camera stands 12.4 m from it looking at azimuth 295, 1.03 m up, matched to the
photograph by the footpads' spacing, the horizon's height and the aft box seen square on (no recovered camera station): the Hasselblad Data
Camera, its 60 mm Biogon and Reseau plate, Ektachrome MS. Scene axes: x east,
y up, z north.
"""
import json
import math
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SCENES = os.path.join(HERE, '..', 'scenes')


def lin(hexcolor):
    """An sRGB hex color as linear values."""
    out = []
    for i in (1, 3, 5):
        c = int(hexcolor[i:i + 2], 16) / 255.0
        out.append(round(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4, 4))
    return out


def scaled(color, factor):
    return [round(c * factor, 4) for c in color]


def anodized(hexcolor):
    """Gray anodized aluminum: diffuse at the full color, reflection under 5%, a brushed grain."""
    c = lin(hexcolor)
    grain = {"type": "noise", "scale": 0.004, "octaves": 3, "seed": 5, "stretch": [60, 1, 1],
             "colors": [scaled(c, 0.88), c]}
    return [{"type": "ambient", "influence": 1.0, "color": scaled(c, 0.02)},
            {"type": "diffuse", "influence": 0.97, "color": [1, 1, 1], "diffuseCast": False, "texture": grain},
            {"type": "metal", "influence": 0.04, "metal": "aluminum", "roughness": 0.4, "tint": c},
            {"type": "specular", "influence": 0.08, "color": [1, 1, 1], "power": 0.35, "indexOfRefraction": 1.5,
             "materialHighlight": False}]


PANEL_IMAGE = {"type": "image", "file": "lm5_eagle_panels.png", "srgb": True}


def panel(hexcolor, gain=1.0):
    """An anodized micrometeoroid-shield panel: satin metal, its color (each panel a little different,
    with its seam and rivets) from the model's panel atlas through its UVs."""
    c = lin(hexcolor)
    return [{"type": "ambient", "influence": 1.0, "color": scaled(c, 0.02)},
            {"type": "diffuse", "influence": round(0.38 * gain, 4), "color": [1, 1, 1], "diffuseCast": False, "texture": PANEL_IMAGE},
            {"type": "metal", "influence": round(0.35 * gain, 4), "metal": "aluminum", "roughness": 0.3, "tint": PANEL_IMAGE},
            {"type": "specular", "influence": 0.3, "color": c, "power": 0.15, "indexOfRefraction": 1.5,
             "materialHighlight": False}]


def stainless():
    """304 stainless steel, polished: its own optical constants, nearly a mirror, with the sun's highlight."""
    return [{"type": "metal", "influence": 0.9, "n": [2.76, 2.55, 2.30], "k": [3.79, 3.55, 3.18], "roughness": 0.12},
            {"type": "specular", "influence": 0.6, "color": [1, 1, 1], "power": 0.12, "indexOfRefraction": 1.5,
             "materialHighlight": False},
            {"type": "diffuse", "influence": 0.06, "color": lin('#8A8C8E'), "diffuseCast": False}]


def kapton(hexcolor):
    """Aluminized Kapton: the copper of the film over its aluminum backing, and the film's own front
    surface (n about 1.7), which reflects about 7% face on, untinted, more at grazing angles."""
    c = lin(hexcolor)
    return [{"type": "metal", "influence": 0.4, "metal": "copper", "roughness": 0.15, "tint": c},
            {"type": "specular", "influence": 0.4, "color": c, "power": 0.15, "indexOfRefraction": 1.5,
             "materialHighlight": False},
            {"type": "diffuse", "influence": 0.06, "color": c, "diffuseCast": False},
            {"type": "reflective", "influence": 0.07, "color": [1, 1, 1], "diffuseReflectivity": 0.0, "roughness": 0.05},
            {"type": "specular", "influence": 0.15, "color": [1, 1, 1], "power": 0.05, "indexOfRefraction": 1.7,
             "materialHighlight": False}]


def metal(hexcolor, metalness, roughness, preset='aluminum'):
    """A metal with the sun's highlight in its own tint (Ray's metal is a pure mirror)."""
    c = lin(hexcolor)
    return [{"type": "metal", "influence": metalness, "metal": preset, "roughness": roughness, "tint": c},
            {"type": "specular", "influence": metalness, "color": c, "power": max(roughness, 0.05),
             "indexOfRefraction": 1.5, "materialHighlight": False},
            {"type": "diffuse", "influence": round(max(1.0 - metalness, 0.05), 3), "color": c, "diffuseCast": False}]


def paint(hexcolor, roughness):
    c = lin(hexcolor)
    return [{"type": "ambient", "influence": 1.0, "color": scaled(c, 0.02)},
            {"type": "diffuse", "influence": 0.92, "color": c, "diffuseCast": False},
            {"type": "specular", "influence": 0.06, "color": [1, 1, 1], "power": roughness, "indexOfRefraction": 1.5,
             "materialHighlight": False}]


CRINKLE = {"mode": "height", "texture": {"type": "cells", "mode": "edges", "scale": 0.02, "turbulence": 0.9, "seed": 7},
           "depth": 0.0025, "step": 0.001}
    # The quadrant blankets' folds are large: many as big as a head (20 to 30 cm,
    # AS11-40-5927 beside Aldrin), a few centimeters deep, with sharp facets
    # (the distance to the nearest cell point makes a facet that rises to a
    # ridge where two cells meet).
BIG_CRINKLE = {"mode": "height", "texture": {"type": "cells", "mode": "edges", "scale": 0.27, "turbulence": 0.3, "seed": 9},
               "depth": 0.05, "step": 0.006}
    # The plume deflectors: gray, gently wrinkled, streaked from top to bottom
    # (noise stretched along the panel's height).
STREAKS = {"type": "noise", "scale": 0.03, "octaves": 3, "seed": 21, "stretch": [1, 40, 1],
           "colors": [lin('#4A4B4C'), lin('#8C8D8E')]}
DEFLECTOR = [{"type": "ambient", "influence": 1.0, "color": [0.003, 0.003, 0.003]},
             {"type": "diffuse", "influence": 0.9, "color": [1, 1, 1], "diffuseCast": False, "texture": STREAKS},
             {"type": "specular", "influence": 0.05, "color": [1, 1, 1], "power": 0.5, "indexOfRefraction": 1.5,
              "materialHighlight": False}]
WRINKLE = {"mode": "height", "texture": {"type": "noise", "scale": 0.12, "octaves": 3, "seed": 4}, "depth": 0.006, "step": 0.008}
RIVETS = {"mode": "height", "texture": {"type": "image", "file": "lm5_eagle_rivets.png"}, "depth": 0.0015, "step": 0.002}
QUARTZ = [{"type": "ambient", "influence": 1.0, "color": [0.004, 0.004, 0.008]},
          {"type": "diffuse", "influence": 0.05, "color": lin('#0B131A'), "diffuseCast": False},
          {"type": "reflective", "influence": 0.32, "color": [0.62, 0.55, 0.95], "diffuseReflectivity": 0.0,
           "filterReflection": True, "roughness": 0.02},
          {"type": "specular", "influence": 0.45, "color": [0.8, 0.75, 1.0], "power": 0.05, "indexOfRefraction": 1.46,
           "materialHighlight": False}]

MATERIAL_MAP = {
    "kapton_blanket": {"materials": kapton('#D6865A'), "bump": BIG_CRINKLE},
    "kapton_gold": {"materials": metal('#DFA53A', 0.65, 0.25), "bump": CRINKLE},
    "silver_foil": {"materials": metal('#D4D8DC', 0.7, 0.25), "bump": CRINKLE},
    "al_olive": {"materials": panel('#7A7966'), "bump": RIVETS},
    "kapton_mesa": {"materials": metal('#E59B34', 0.65, 0.25), "bump": CRINKLE},
    "black_blanket": {"materials": paint('#1C1D1F', 0.6), "bump": BIG_CRINKLE},
    "dps_bell": {"materials": metal('#3A3D42', 0.85, 0.45, 'titanium')},
    "porch_aluminum": {"materials": metal('#34373A', 0.65, 0.3)},
    "rcs_stainless": {"materials": stainless()},
    "plume_deflector": {"materials": DEFLECTOR, "bump": WRINKLE},
    "inconel_dark": {"materials": metal('#26282A', 0.65, 0.55, 'iron')},
    "white_paint": {"materials": paint('#E2E4E6', 0.45)},
    "al2024_anodized": {"materials": panel('#C2D4FF', 1.1), "bump": RIVETS},
    "al5056_anodized": {"materials": panel('#F1F4F2', 1.1), "bump": RIVETS},
    "pyromark_black": {"materials": paint('#18191B', 0.7)},
    "window_quartz": {"materials": QUARTZ},
}

SUN_ELEVATION = 14.96
SUN_AZIMUTH = 90.2
LM_HEADING = 244.0          # the azimuth the LM's front faces: its aft equipment bay to the right, as in the photograph
CAMERA_HEADING = 295.0
CAMERA_DISTANCE = 12.4
    # Ektachrome MS at 1/250 s: the stop that gives the sunlit ground the photograph's brightness.
FILM_FSTOP = 8.6


def toward(azimuth, elevation=0.0):
    a, e = math.radians(azimuth), math.radians(elevation)
    return [math.cos(e) * math.sin(a), math.sin(e), math.cos(e) * math.cos(a)]


def r6(values):
    return [round(v, 6) for v in values]


def base_scene(width, height, camera, objects, lights):
    return {"format": "Ray scene", "version": 3,
            "settings": {"imageWidth": width, "imageHeight": height, "imageFile": "eagle.out", "colorDepth": 8,
                         "maximumDepth": 6, "depthThreshold": 0.01,
                         "indirect": {"method": "hemisphere", "bounces": 2, "samples": 16, "distance": 20.0,
                                      "strength": 1.0}},
            "background": [0, 0, 0], "camera": camera, "objects": objects, "lights": lights}


def lm_frame(point, heading=LM_HEADING):
    """A point in the LM's frame (Ray reads its front as -z) into the scene, the LM turned to face heading."""
    # Turning the model about y by t degrees takes its front (-z) to azimuth heading.
    t = math.radians(heading - 180.0)
    x, y, z = point
    return [x * math.cos(t) + z * math.sin(t), y, -x * math.sin(t) + z * math.cos(t)]


def main():
    prefab_mesh = {"type": "mesh", "name": "lm5_eagle", "file": "lm5_eagle.glb", "scale": 1.0, "rotation": [0, 0, 0],
                   "align": "none", "translation": [0, 0, 0], "smooth": True, "doubleSided": True,
                   "color": [0.5, 0.5, 0.5], "materials": anodized('#B8BDC2'), "materialMap": MATERIAL_MAP}
    prefab_camera = {"position": [10.0, 2.0, -12.0], "lookAt": [0.0, 2.6, 0.0], "up": [0, 1, 0],
                     "verticalAngle": 40.0, "horizontalAngle": 40.0, "aspectRatio": 1.0}
    sun = {"type": "directional", "direction": r6([-v for v in toward(SUN_AZIMUTH, SUN_ELEVATION)]),
           "color": [2.2, 2.2, 2.15], "angle": 0.53, "samples": 2}
    prefab = base_scene(256, 256, prefab_camera, [prefab_mesh], [sun])
    with open(os.path.join(SCENES, 'models', 'apollo', 'lm5_eagle.ray'), 'w') as f:
        json.dump(prefab, f, indent=2)

    ground = {"type": "plane", "position": [0, 0, 0], "normal": [0, 1, 0], "color": [0.5, 0.5, 0.5],
              "materials": [{"type": "regolith", "influence": 1.0, "color": [0.12, 0.115, 0.11], "surge": 1.0, "width": 0.06}],
              "bump": {"mode": "height", "texture": {"type": "noise", "scale": 0.9, "octaves": 6, "seed": 11},
                       "depth": 0.05, "step": 0.005}}
    look = toward(CAMERA_HEADING)
    camera = {"position": r6([-CAMERA_DISTANCE * look[0], 1.03, -CAMERA_DISTANCE * look[2]]), "lookAt": [0.114, 1.67, 0.245],
              "up": [0, 1, 0], "body": "hasselblad-hdc",
              "sampler": {"type": "stratified", "grid": 2, "seed": 0},
              "colorModel": {"type": "film", "stock": "ektachrome-ms", "shutter": 0.004, "fStop": FILM_FSTOP},
              "lens": {"focusDistance": CAMERA_DISTANCE, "millimetersPerUnit": 1000.0}}
    # 4 cm down: the footpads sank that far into the regolith.
    eagle = {"type": "include", "file": "models/apollo/lm5_eagle.ray", "rotation": [0.0, round(LM_HEADING - 180.0, 6), 0.0],
             "position": [0.0, -0.04, 0.0]}
    rock = paint('#7E7B76', 0.8)
    near = [-CAMERA_DISTANCE * look[0] * 0.55, 0.0, -CAMERA_DISTANCE * look[2] * 0.55]
    props = [
            # A rock about 70 cm across and low in the middle foreground, half sunk in the ground.
        {"type": "superquadric", "center": r6([near[0], 0.0, near[2]]), "radius": [0.36, 0.12, 0.28], "e1": 1.6, "e2": 1.4,
         "rotation": [0, 30, 0], "color": [0.5, 0.5, 0.5], "materials": rock,
         "bump": {"mode": "height", "texture": {"type": "noise", "scale": 0.08, "octaves": 4, "seed": 2}, "depth": 0.03, "step": 0.004}},
            # The white bag near the left footpad, and a pole at the left.
        {"type": "cylinder", "base": r6(lm_frame([2.6, 0.0, -1.7], LM_HEADING)), "top": r6(lm_frame([2.6, 0.32, -1.7], LM_HEADING)),
         "radius": 0.13, "capped": True, "color": [0.5, 0.5, 0.5], "materials": paint('#E8E8E6', 0.5)},
        {"type": "cylinder", "base": r6(lm_frame([3.6, 0.0, -0.4], LM_HEADING)), "top": r6(lm_frame([3.6, 0.9, -0.4], LM_HEADING)),
         "radius": 0.012, "capped": True, "color": [0.5, 0.5, 0.5], "materials": anodized('#B8BDC2')},
    ]
    objects = [ground, eagle] + props
    scene = base_scene(128, 128, camera, objects, [sun])
    with open(os.path.join(SCENES, 'as11_40_5927_test.ray'), 'w') as f:
        json.dump(scene, f, indent=2)
    print("wrote lm5_eagle.ray and as11_40_5927_test.ray")


if __name__ == '__main__':
    main()
