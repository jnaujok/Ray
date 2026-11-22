#!/usr/bin/env python3
"""Solves the AS11-40-5927 camera and the LM's heading from the photograph.

    solve_5927_view.py

The four footpads' places in NASA's frame (measured on the 1950 x 1950 scan)
fix the view: the LM's heading, the camera's distance from the LM, and its aim
(pan and tilt), with the camera's azimuth from the LM set by the LM's shadow
(CAMERA_AZIMUTH). The camera stands 1.03 m above the ground
under it (its height on the RCU bracket), with that ground read from the LROC
NAC DTM crop in scenes/terrain/ (tools/geotiff_terrain.py), and the LM sits
on the DTM's plane under it, tilted to match, its pads 4 cm into the
regolith. The lens is the Data Camera's 61.1 mm Biogon on its 53 mm format,
taken as a pinhole (its distortion is under 0.6%).

Prints the solved values, which tools/make_eagle_scenes.py uses, and the
pads' places in the photograph and in the solved view.

Only the Python standard library and numpy are used.
"""
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import geotiff_terrain  # noqa: E402

CROP = os.path.join(HERE, '..', 'scenes', 'terrain', 'apollo11_inner_crop.tif')
LM_LAT, LM_LON = 0.67416, 23.47314
PHOTO = 1950.0
# Footpads in the photograph (pixels), and in the model (LM frame, front -z).
PADS_PHOTO = {'near left': (290.0, 1312.0), 'near right': (1645.0, 1305.0),
              'far right': (1440.0, 1190.0), 'far left (ladder)': (625.0, 1135.0)}
PAD_WEIGHT = {'near left': (1, 1), 'near right': (1, 1), 'far right': (1, 1), 'far left (ladder)': (1, 0.3)}
PADS_MODEL = {'+x': (4.006, 0.028, 0.058), '-x': (-4.006, 0.028, 0.058), '+z': (-0.049, 0.028, 4.005),
              '-z': (-0.05, 0.028, -4.005)}
FOCAL, FORMAT = 61.1, 53.0
    # The pads fix the LM's heading less the camera's azimuth, not the azimuth
    # itself (turning both together about the LM moves them only by the
    # terrain's tilt). The LM's shadow fixes it: from 115 degrees, the shadow
    # (cast toward azimuth 270.2) reaches out past the left footpad as far as it
    # does in the photograph (renders 5 degrees either way miss it visibly).
CAMERA_AZIMUTH = 115.0
FILL = 9.0      # the radius geotiff_terrain.py --fill-center flattened (its plane is fitted from FILL to 2 FILL)
EYE = 1.03
SINK = 0.04


def rotate_xyz(v, rx, ry, rz):
    """Ray's RotateXYZ (core/src/LocalShape.cpp): about x, then y, then z, in radians."""
    x, y, z = v
    y, z = y * math.cos(rx) - z * math.sin(rx), y * math.sin(rx) + z * math.cos(rx)
    x, z = x * math.cos(ry) + z * math.sin(ry), z * math.cos(ry) - x * math.sin(ry)
    x, y = x * math.cos(rz) - y * math.sin(rz), x * math.sin(rz) + y * math.cos(rz)
    return np.array([x, y, z])


def terrain():
    grid = geotiff_terrain.read_any(CROP)
    east, north, heights, center = geotiff_terrain.local_grid(grid, LM_LAT, LM_LON, 30.0, 1)
    E, N = np.meshgrid(east, north)
    ring = (E * E + N * N >= FILL ** 2) & (E * E + N * N <= (2.0 * FILL) ** 2)
    A = np.stack([np.ones(int(ring.sum())), E[ring], N[ring]], axis=1)
    (c0, ce, cn), *_ = np.linalg.lstsq(A, heights[ring], rcond=None)

    def ground(x, z):
        if x * x + z * z > 2500.0:
            return float('nan')
        r, c = grid.to_pixel(LM_LAT + math.degrees(z / geotiff_terrain.MOON_RADIUS),
                             LM_LON + math.degrees(x / (geotiff_terrain.MOON_RADIUS * math.cos(math.radians(LM_LAT)))))
        return geotiff_terrain.bilinear(grid.heights, r, c) - (center + c0)
    return ground, ce, cn


def lm_rotation(heading, ce, cn):
    """Euler angles (degrees, Ray's order) equal to: turn the LM to face heading, then tilt it onto the plane."""
    want = tilt_matrix(ce, cn) @ np.column_stack([rotate_xyz(e, 0.0, math.radians(heading - 180.0), 0.0) for e in np.eye(3)])

    def error(p):
        rx, ry, rz = np.radians(p)
        got = np.column_stack([rotate_xyz(e, rx, ry, rz) for e in np.eye(3)])
        return float(np.sum((got - want) ** 2))
    return nelder_mead(error, np.array([0.0, heading - 180.0, 0.0]), 1.0, 3000)


def nelder_mead(f, x0, step, iterations):
    n = len(x0)
    pts = [x0] + [x0 + np.eye(n)[i] * step for i in range(n)]
    vals = [f(p) for p in pts]
    for _ in range(iterations):
        order = np.argsort(vals)
        pts = [pts[i] for i in order]
        vals = [vals[i] for i in order]
        centroid = np.mean(pts[:-1], axis=0)
        r = centroid + (centroid - pts[-1])
        fr = f(r)
        if fr < vals[0]:
            e = centroid + 2.0 * (centroid - pts[-1])
            fe = f(e)
            pts[-1], vals[-1] = (e, fe) if fe < fr else (r, fr)
        elif fr < vals[-2]:
            pts[-1], vals[-1] = r, fr
        else:
            c = centroid + 0.5 * (pts[-1] - centroid)
            fc = f(c)
            if fc < vals[-1]:
                pts[-1], vals[-1] = c, fc
            else:
                pts = [pts[0]] + [pts[0] + 0.5 * (p - pts[0]) for p in pts[1:]]
                vals = [vals[0]] + [f(p) for p in pts[1:]]
    i = int(np.argmin(vals))
    return pts[i]


def project(point, camera, target):
    forward = target - camera
    forward /= np.linalg.norm(forward)
    right = np.cross(np.array([0.0, 1.0, 0.0]), forward)   # Ray: right = up x forward (left-handed axes)
    right /= np.linalg.norm(right)
    top = np.cross(forward, right)
    d = point - camera
    z = d @ forward
    half = (FORMAT / 2.0) / FOCAL
    return (0.5 + 0.5 * (d @ right) / z / half) * PHOTO, (0.5 - 0.5 * (d @ top) / z / half) * PHOTO


def tilt_matrix(ce, cn):
    """The rotation (Rodrigues) that takes straight up to the normal of the plane y = ce x + cn z."""
    n = np.array([-ce, 1.0, -cn])
    n /= np.linalg.norm(n)
    up = np.array([0.0, 1.0, 0.0])
    axis = np.cross(up, n)
    s, c = np.linalg.norm(axis), float(up @ n)
    if s < 1e-12:
        return np.eye(3)
    k = axis / s
    K = np.array([[0, -k[2], k[1]], [k[2], 0, -k[0]], [-k[1], k[0], 0]])
    return np.eye(3) + s * K + (1 - c) * (K @ K)


def setup(params, ground, ce, cn, tilt_m):
    heading, azimuth, distance, pan, tilt = params
    yaw = math.radians(heading - 180.0)
    cx, cz = distance * math.sin(math.radians(azimuth)), distance * math.cos(math.radians(azimuth))
    camera = np.array([cx, ground(cx, cz) + EYE, cz])
    view = math.radians(azimuth + 180.0 + pan)
    target = camera + np.array([math.sin(view), math.tan(math.radians(tilt)), math.cos(view)])
    pads = {k: tilt_m @ rotate_xyz(v, 0.0, yaw, 0.0) + np.array([0.0, -SINK, 0.0]) for k, v in PADS_MODEL.items()}
    return camera, target, pads


def match(params, ground, ce, cn, tilt_m):
    camera, target, pads = setup(params, ground, ce, cn, tilt_m)
    pts = {k: project(p, camera, target) for k, p in pads.items()}
    # Name the model's pads by where they land: two nearest, then left/right.
    by_depth = sorted(pts, key=lambda k: np.linalg.norm(pads[k] - camera))
    near = sorted(by_depth[:2], key=lambda k: pts[k][0])
    far = sorted(by_depth[2:], key=lambda k: pts[k][0])
    named = {'near left': pts[near[0]], 'near right': pts[near[1]], 'far left (ladder)': pts[far[0]],
             'far right': pts[far[1]]}
    return named, {'near left': near[0], 'near right': near[1], 'far left (ladder)': far[0], 'far right': far[1]}


def cost(params, ground, ce, cn, tilt_m):
    if not 5.0 <= params[2] <= 40.0 or abs(params[3]) > 25.0 or abs(params[4]) > 25.0:
        return 1e12
    named, which = match(params, ground, ce, cn, tilt_m)
    if any(math.isnan(v[0]) for v in named.values()):
        return 1e12
    if which['far left (ladder)'] != '-z':       # the ladder leg is the LM's front, -z
        return 1e12
    total = 0.0
    for k, (u, v) in PADS_PHOTO.items():
        wu, wv = PAD_WEIGHT[k]
        total += wu * (named[k][0] - u) ** 2 + wv * (named[k][1] - v) ** 2
    return total


def main():
    ground, ce, cn = terrain()
    T = tilt_matrix(ce, cn)
    print('ground plane under the LM: %.2f degrees, downhill toward azimuth %.1f' %
          (math.degrees(math.atan(math.hypot(ce, cn))), math.degrees(math.atan2(-ce, -cn)) % 360.0))

    def full(p):
        return np.array([p[0], CAMERA_AZIMUTH, p[1], p[2], p[3]])

    def f(p):
        return cost(full(p), ground, ce, cn, T)
    best = None
    for heading in range(200, 300, 5):
        x0 = np.array([float(heading), 12.4, 0.0, 3.0])
        x = nelder_mead(f, x0, 3.0, 600)
        if best is None or f(x) < best[0]:
            best = (f(x), x)
    x = full(nelder_mead(f, best[1], 0.5, 3000))
    c = cost(x, ground, ce, cn, T)
    heading, azimuth, distance, pan, tilt = x
    camera, target, pads = setup(x, ground, ce, cn, T)
    rot = lm_rotation(heading, ce, cn)
    named, which = match(x, ground, ce, cn, T)
    print('LM heading %.2f deg; LM rotation (Ray, x/y/z) %s' % (heading, np.round(rot, 3).tolist()))
    print('camera: azimuth %.2f deg from the LM, %.2f m out, ground there %+.3f m, eye %.3f m above the LM ground'
          % (azimuth, distance, camera[1] - EYE, camera[1]))
    print('camera position %s, look-at %s (pan %.2f, tilt %.2f deg)' %
          (np.round(camera, 4).tolist(), np.round(camera + 10 * (target - camera) / np.linalg.norm(target - camera), 4).tolist(), pan, tilt))
    print('rms pad error %.1f px (of %d)' % (math.sqrt(c / 8.0), PHOTO))
    for k, (u, v) in PADS_PHOTO.items():
        print('  %-18s photo (%6.1f, %6.1f)  solved (%6.1f, %6.1f)  model pad %s' % (k, u, v, named[k][0], named[k][1], which[k]))


if __name__ == '__main__':
    main()
