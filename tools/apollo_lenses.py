#!/usr/bin/env python3
"""Works out Ray's built-in Apollo lens prescriptions (core/src/Lens.cpp).

    apollo_lenses.py [--optimize] [--table]

Each lens is a list of surfaces (radius, thickness, glass) at some focal
length, from a patent's worked example or, where none was published, a
reconstruction optimized here. The script scales each to its nominal focal
length, works out the semi-aperture of every surface (the widest the axial
and full-field pencils reach), and reports the focal length, back focus,
f-number and distortion it measures by real ray tracing. --table prints the
C++ table Lens.cpp holds; --optimize reruns the reconstructions' optimizer
from their starting points (the results are already written below).

Glasses are (nd, Vd) pairs, traced with the same Cauchy curve as Ray, or a
Sellmeier glass by name ("silica", "caf2"; Malitson 1965 and 1963).

Only the Python standard library and NumPy are needed.
"""
import math
import sys

import numpy as np

WL_C, WL_D, WL_F = 0.65627, 0.58756, 0.48613

SELLMEIER = {
    # I. H. Malitson, "Interspecimen comparison of the refractive index of
    # fused silica", JOSA 55 (1965): 0.21-3.71 um.
    "silica": ((0.6961663, 0.4079426, 0.8974794), (0.0684043, 0.1162414, 9.896161)),
    # I. H. Malitson, "A redetermination of some optical properties of
    # calcium fluoride", Applied Optics 2 (1963): 0.23-9.7 um.
    "caf2": ((0.5675888, 0.4710914, 3.8484723), (0.050263605, 0.1003909, 34.649040)),
}


def sellmeier(name, um):
    b, c = SELLMEIER[name]
    l2 = um * um
    return math.sqrt(1.0 + sum(bi * l2 / (l2 - ci * ci) for bi, ci in zip(b, c)))


def index(glass, um):
    if glass is None:
        return 1.0
    if isinstance(glass, str):
        return sellmeier(glass, um)
    nd, v = glass
    b = (nd - 1.0) / (v * (1.0 / WL_F ** 2 - 1.0 / WL_C ** 2))
    a = nd - b / WL_D ** 2
    return a + b / um ** 2


# A surface: [radius, thickness after it, glass after it (None for air),
# stop?]. Radii follow the patents' sign convention, which is Ray's:
# positive when the center of curvature is behind the surface.


def S(r, d, glass=None, stop=False):
    return [float(r), float(d), glass, stop]


def stop(d):
    return S(0.0, d, None, True)


# --- Patent examples -----------------------------------------------------

# US 2,721,499 (L. Bertele, filed 1952): "Five component wide-angle
# objective", Example 2, f = 100, 1:4.5, the 8-element, 5-group Biogon of
# the Hasselblad SWC. The stop is in the third air space (l3 = 4.7).
BIOGON = [
    S(109.14, 3.7, (1.50380, 66.7)), S(52.63, 13.2),
    S(110.25, 3.7, (1.48697, 70.3)), S(50.72, 35.0),
    S(56.24, 29.3, (1.72050, 50.3)), S(25.37, 13.3, (1.60739, 59.5)), S(-194.92, 2.35),
    stop(2.35),
    S(-252.70, 2.8, (1.56993, 57.5)), S(30.59, 23.7, (1.62500, 53.3)), S(-25.51, 18.2, (1.71966, 29.3)),
    S(-57.38, 39.0),
    S(-40.15, 9.8, (1.64200, 58.1)), S(-102.64, 0.0),
]

# US 2,724,994 (G. Lange, Carl Zeiss, priority 1953): "Photographic
# objective comprising four meniscus shaped air spaced components", f = 100,
# 1:2.8, five elements in four groups: the Planar 80 mm f/2.8 formula.
# Example I; the stop lies between b1 = 6.270 and b2 = 14.329.
PLANAR_I = [
    S(44.284, 1.872, (1.62536, 35.6)), S(26.761, 7.863, (1.62041, 60.3)), S(96.327, 0.125),
    S(29.174, 11.682, (1.76182, 26.5)), S(18.945, 6.270),
    stop(14.329),
    S(-17.885, 2.621, (1.76182, 26.5)), S(-23.509, 0.125),
    S(-112.85, 7.788, (1.62041, 60.3)), S(-27.941, 0.0),
]

# The same patent's Example II (f = 100, 1:2.8), used for the 100 mm
# f/3.5 Planar family member: see LENSES below.
PLANAR_II = [
    S(44.341, 2.624, (1.69895, 30.1)), S(29.212, 6.998, (1.69347, 53.5)), S(84.733, 0.125),
    S(29.635, 11.597, (1.69895, 30.1)), S(18.969, 6.270),
    stop(14.225),
    S(-17.908, 2.437, (1.76182, 26.5)), S(-23.539, 0.250),
    S(-111.38, 7.536, (1.62041, 60.3)), S(-27.577, 0.0),
]

# --- Reconstructions -------------------------------------------------------
# No prescription was published for these. Each is a layout matching the
# lens's published construction, optimized here (--optimize) for a small
# spot over its field in C, d and F light, at f = 100.

# Sonnar 250 mm f/5.6: 4 elements in 3 groups, a front collective crown
# lens, a cemented crown-flint doublet, the stop, and a rear dispersive
# meniscus (a long-focus Sonnar). Glasses: BK7, SK4, F2, SF5 (Schott).
SONNAR_START = [
    S(48.0, 3.2, (1.51680, 64.17)), S(-400.0, 0.4),
    S(28.0, 4.0, (1.61272, 58.63)), S(-120.0, 1.6, (1.62004, 36.37)), S(55.0, 12.0),
    stop(14.0),
    S(-18.0, 1.4, (1.67270, 32.21)), S(-30.0, 0.0),
]

# Tele-Tessar 500 mm f/8: 5 elements in 3 groups, a cemented front doublet,
# a collective meniscus, the stop well back, and a cemented dispersive
# telephoto rear doublet. Glasses: SK16, F2, BK7, SF5, SK4 (Schott).
TELETESSAR_START = [
    S(30.0, 2.0, (1.62041, 60.32)), S(-60.0, 0.8, (1.62004, 36.37)), S(-250.0, 0.3),
    S(22.0, 1.6, (1.51680, 64.17)), S(36.0, 20.0),
    stop(14.0),
    S(-16.0, 0.8, (1.67270, 32.21)), S(30.0, 1.4, (1.61272, 58.63)), S(-40.0, 0.0),
]

# UV-Sonnar 105 mm f/4.3: 7 air-spaced singlets of fused silica and
# calcium fluoride (no cement, which fluoresces and absorbs ultraviolet),
# four ahead of the stop and three behind, in a Sonnar's power layout.
UVSONNAR_START = [
    S(40.0, 4.0, "caf2"), S(-300.0, 0.3),
    S(26.0, 4.0, "caf2"), S(120.0, 0.6),
    S(220.0, 1.6, "silica"), S(18.0, 2.0),
    S(30.0, 3.0, "caf2"), S(80.0, 6.0),
    stop(6.0),
    S(-22.0, 1.4, "silica"), S(60.0, 0.8),
    S(90.0, 3.2, "caf2"), S(-28.0, 0.3),
    S(-150.0, 2.4, "caf2"), S(-45.0, 0.0),
]

# The results of --optimize (f = 100), written back here.
SONNAR = [
    S(55.1428, 3.4824, (1.5168, 64.17), False),
    S(136.2435, 0.4353, None, False),
    S(29.8545, 4.3529, (1.61272, 58.63), False),
    S(-103.2410, 1.7412, (1.62004, 36.37), False),
    S(43.6525, 13.0588, None, False),
    S(0.0000, 15.2353, None, True),
    S(-18.6708, 1.5235, (1.6727, 32.21), False),
    S(-22.7519, 0.0000, None, False),
]
TELETESSAR = [
    S(36.1743, 1.7654, (1.62041, 60.32), False),
    S(-47.0632, 0.7061, (1.62004, 36.37), False),
    S(98.7950, 0.2648, None, False),
    S(21.2055, 1.4123, (1.5168, 64.17), False),
    S(31.6412, 17.6536, None, False),
    S(0.0000, 12.3576, None, True),
    S(-15.3765, 0.7061, (1.6727, 32.21), False),
    S(28.0159, 1.2358, (1.61272, 58.63), False),
    S(-30.9025, 0.0000, None, False),
]
UVSONNAR = [
    S(53.7315, 5.3107, 'caf2', False),
    S(-13306.4019, 0.3983, None, False),
    S(33.6375, 5.3107, 'caf2', False),
    S(141.0015, 0.7966, None, False),
    S(335.1947, 2.1243, 'silica', False),
    S(24.6376, 2.6554, None, False),
    S(41.1497, 3.9831, 'caf2', False),
    S(94.2303, 7.9661, None, False),
    S(0.0000, 7.9661, None, True),
    S(-31.6961, 1.8588, 'silica', False),
    S(68.9031, 1.0621, None, False),
    S(134.7273, 4.2486, 'caf2', False),
    S(-33.4120, 0.3983, None, False),
    S(-299.4306, 3.1864, 'caf2', False),
    S(-52.0908, 0.0000, None, False),
]


# --- Ray tracing -----------------------------------------------------------

def vertices(surfs):
    z, out = 0.0, []
    for s in surfs:
        out.append(z)
        z += s[1]
    return out


def trace(surfs, origin, direction, um, until=None):
    """Traces a ray through the surfaces; returns (points, final origin,
    direction) or None if it misses or reflects totally."""
    o = np.array(origin, float)
    d = np.array(direction, float)
    d /= np.linalg.norm(d)
    zs = vertices(surfs)
    n0 = 1.0
    pts = []
    count = len(surfs) if until is None else until
    for i in range(count):
        r, _, glass, _ = surfs[i]
        z0 = zs[i]
        if r == 0.0:
            if d[2] == 0:
                return None
            t = (z0 - o[2]) / d[2]
            p = o + t * d
            nrm = np.array([0.0, 0.0, -1.0])
        else:
            c = np.array([0.0, 0.0, z0 + r])
            oc = o - c
            b = np.dot(oc, d)
            cc = np.dot(oc, oc) - r * r
            disc = b * b - cc
            if disc < 0:
                return None
            sq = math.sqrt(disc)
            # Rays run toward +z: a surface whose center is behind it is the
            # sphere's near side, one whose center is in front its far side.
            t = (-b - sq) if r > 0 else (-b + sq)
            if t <= 1e-12:
                return None
            p = o + t * d
            nrm = (p - c) / r
            if nrm[2] > 0:
                nrm = -nrm
        n1 = index(glass, um)
        eta = n0 / n1
        cosi = -np.dot(nrm, d)
        k = 1 - eta * eta * (1 - cosi * cosi)
        if k < 0:
            return None
        d = eta * d + (eta * cosi - math.sqrt(k)) * nrm
        o = p
        pts.append(p.copy())
        n0 = n1
    return pts, o, d


def paraxial(surfs, um=WL_D, h=1e-4):
    """Effective focal length and back focal distance (from the last
    vertex), by a near-axis ray from infinity."""
    res = trace(surfs, [0.0, h, -1.0], [0.0, 0.0, 1.0], um)
    if res is None:
        return None, None
    _, o, d = res
    efl = -h / (d[1] / d[2])
    tz = o[2] - o[1] * d[2] / d[1]
    bfd = tz - vertices(surfs)[-1]
    return efl, bfd


def stop_index(surfs):
    for i, s in enumerate(surfs):
        if s[3]:
            return i
    raise ValueError("no stop")


def stop_radius(surfs, fnum):
    efl, _ = paraxial(surfs)
    h = efl / (2.0 * fnum) * 1e-3
    res = trace(surfs, [0.0, h, -1.0], [0.0, 0.0, 1.0], WL_D, until=stop_index(surfs) + 1)
    return abs(res[0][-1][1]) * 1e3


def aim(surfs, angle, height_at_stop, um=WL_D):
    """A ray at angle (radians) from the object side through the stop at
    height_at_stop (meridional), found by secant iteration on its entry
    height."""
    si = stop_index(surfs)
    zs = vertices(surfs)
    def at_stop(y0):
        o = [0.0, y0, zs[0] - 10.0]
        dvec = [0.0, math.sin(angle), math.cos(angle)]
        res = trace(surfs, o, dvec, um, until=si + 1)
        if res is None:
            return None
        return res[0][-1][1]
    # A first guess through the paraxial entrance pupil.
    ze = zs[0] - 10.0
    a = 1e-5
    ya = at_stop(0.0) if False else None
    # where a slightly tilted ray must enter to pass the stop's center
    g0 = _secant(lambda y: _stop_y(surfs, si, ze, a, y), 0.0, 1e-3, 0.0)
    zep = ze - g0 / math.tan(a) if g0 is not None else zs[si]
    hs = _stop_y(surfs, si, ze, 0.0, 1e-4)
    mag = (1e-4 / hs) if hs else 1.0
    y_guess = height_at_stop * mag + math.tan(angle) * (ze - zep)
    y = _secant(at_stop, y_guess, y_guess + 1e-3 * (1.0 + abs(y_guess)), height_at_stop)
    if y is None:
        y = y_guess
        if at_stop(y) is None:
            return None
    return [0.0, y, ze], [0.0, math.sin(angle), math.cos(angle)]


def _stop_y(surfs, si, ze, angle, y0):
    res = trace(surfs, [0.0, y0, ze], [0.0, math.sin(angle), math.cos(angle)], WL_D, until=si + 1)
    return None if res is None else res[0][-1][1]


def _secant(f, y0, y1, target):
    f0, f1 = f(y0), f(y1)
    for _ in range(30):
        if f0 is None or f1 is None or f1 == f0:
            return None
        y2 = y1 + (target - f1) * (y1 - y0) / (f1 - f0)
        y0, f0 = y1, f1
        y1, f1 = y2, f(y2)
        if f1 is not None and abs(f1 - target) < 1e-11:
            return y1
    return y1 if f1 is not None and abs(f1 - target) < 1e-6 else None


def image_height(surfs, angle, image_z):
    r = aim(surfs, angle, 0.0)
    if r is None:
        return None
    res = trace(surfs, r[0], r[1], WL_D)
    if res is None:
        return None
    _, o, d = res
    return o[1] + (image_z - o[2]) * d[1] / d[2]


def apertures(surfs, fnum, half_field):
    """Each surface's semi-aperture: the furthest the axial pencil, and the
    full-field pencil vignetted to 60%, reach, plus 3%."""
    sr = stop_radius(surfs, fnum)
    reach = [0.0] * len(surfs)
    for frac_field in (0.0, 1.0):
        ang = half_field * frac_field
        for frac in (-1.0, 0.0, 1.0):
            rim = 1.0 if frac_field == 0.0 else 0.6
            r = aim(surfs, ang, frac * sr * rim)
            if r is None:
                continue
            res = trace(surfs, r[0], r[1], WL_D)
            if res is None:
                continue
            for i, p in enumerate(res[0]):
                reach[i] = max(reach[i], abs(p[1]))
    out = []
    for i, s in enumerate(surfs):
        a = reach[i] * 1.03 + 0.1
        if s[3]:
            a = sr * 1.0
        if s[0] != 0.0:
            a = min(a, abs(s[0]) * 0.98)
        out.append(a)
    return out, sr


def scaled(surfs, focal):
    efl, _ = paraxial(surfs)
    k = focal / efl
    return [[s[0] * k, s[1] * k, s[2], s[3]] for s in surfs]


def spot(surfs, fnum, half_field, fields=(0.0, 0.7, 1.0), wls=(WL_C, WL_D, WL_F), n=5, strict=True):
    """RMS spot radius (over fields and wavelengths, about each field's d
    chief ray), at the best d-light focus near the paraxial one."""
    efl, bfd = paraxial(surfs)
    if efl is None or efl <= 0:
        return 1e9
    sr = stop_radius(surfs, fnum)
    zimg = vertices(surfs)[-1] + bfd
    tot, cnt = 0.0, 0
    for fr in fields:
        ang = half_field * fr
        chief = aim(surfs, ang, 0.0)
        if chief is None:
            return 1e9
        res = trace(surfs, chief[0], chief[1], WL_D)
        if res is None:
            return 1e9
        _, o, d = res
        yc = o[1] + (zimg - o[2]) * d[1] / d[2]
        for wl in wls:
            for i in range(n):
                for j in range(n):
                    u = (i + 0.5) / n * 2 - 1
                    v = (j + 0.5) / n * 2 - 1
                    if u * u + v * v > 1:
                        continue
                    # aim through the stop: meridional offset by u, sagittal by v
                    r = aim(surfs, ang, u * sr)
                    if r is None:
                        if strict:
                            tot += 1e3
                            cnt += 1
                        continue
                    o0 = np.array(r[0])
                    # sagittal: shift entry point in x by v * entrance pupil radius
                    o0[0] = v * efl / (2.0 * fnum)
                    res = trace(surfs, o0, r[1], wl)
                    if res is None:
                        if strict:
                            tot += 1e3
                            cnt += 1
                        continue
                    _, o, d = res
                    x = o[0] + (zimg - o[2]) * d[0] / d[2]
                    y = o[1] + (zimg - o[2]) * d[1] / d[2]
                    tot += x * x + (y - yc) ** 2
                    cnt += 1
    return math.sqrt(tot / max(cnt, 1))


def optimize(surfs, fnum, half_field, extra=None, iters=60):
    """Damped least squares on the curvatures, rescaling to f = 100 after
    every step; extra(surfs) adds penalties (e.g. back focus)."""
    free = [i for i, s in enumerate(surfs) if s[0] != 0.0]
    def merit(x):
        trial = [list(s) for s in surfs]
        for k, i in enumerate(free):
            trial[i][0] = 1.0 / x[k] if x[k] != 0 else 1e9
        try:
            trial = scaled(trial, 100.0)
            m = spot(trial, fnum, half_field, n=4)
            if extra:
                m += extra(trial)
            # edge thickness must stay positive
            ap, _ = apertures(trial, fnum, half_field)
            for i in range(len(trial) - 1):
                if trial[i][2] is None:
                    continue
                def sag(r, a):
                    if r == 0:
                        return 0.0
                    return r - math.copysign(math.sqrt(max(r * r - a * a, 0.0)), r)
                a = max(ap[i], ap[i + 1])
                edge = trial[i][1] - sag(trial[i][0], a) + sag(trial[i + 1][0], a)
                if edge < 0.3:
                    m += (0.3 - edge) * 2.0
        except Exception:
            return 1e9, None
        return m, trial
    x = np.array([1.0 / surfs[i][0] for i in free])
    best, _ = merit(x)
    lam = 1e-3
    for it in range(iters):
        g = np.zeros_like(x)
        h = 1e-6
        for k in range(len(x)):
            xp = x.copy()
            xp[k] += h
            g[k] = (merit(xp)[0] - best) / h
        improved = False
        for _ in range(12):
            step = -g / (np.linalg.norm(g) + 1e-12) * lam
            m, _ = merit(x + step)
            if m < best:
                x, best = x + step, m
                lam *= 1.6
                improved = True
                break
            lam *= 0.4
        if not improved and lam < 1e-9:
            break
        print(f"  iter {it}: merit {best:.5f}", file=sys.stderr)
    _, trial = merit(x)
    return trial


def residuals(surfs, fnum, half_field, fields=(0.0, 0.5, 0.75, 1.0), wls=(WL_C, WL_D, WL_F), n=4):
    """Each traced ray's x and y miss from its field's d chief ray at the
    paraxial focus, divided by the focal length; a lost ray counts 0.05."""
    efl, bfd = paraxial(surfs)
    out = []
    if efl is None or efl <= 0:
        return None
    sr = stop_radius(surfs, fnum)
    zimg = vertices(surfs)[-1] + bfd
    for fr in fields:
        ang = half_field * fr
        chief = aim(surfs, ang, 0.0)
        res = trace(surfs, chief[0], chief[1], WL_D) if chief else None
        if res is None:
            return None
        _, o, d = res
        yc = o[1] + (zimg - o[2]) * d[1] / d[2]
        for wl in wls:
            for i in range(n):
                for j in range(n):
                    u = (i + 0.5) / n * 2 - 1
                    v = (j + 0.5) / n * 2 - 1
                    if u * u + v * v > 1:
                        continue
                    r = aim(surfs, ang, u * sr)
                    res = None
                    if r is not None:
                        o0 = np.array(r[0])
                        o0[0] = v * efl / (2.0 * fnum)
                        res = trace(surfs, o0, r[1], wl)
                    if res is None:
                        out += [0.05, 0.05]
                        continue
                    _, o, d = res
                    x = o[0] + (zimg - o[2]) * d[0] / d[2]
                    y = o[1] + (zimg - o[2]) * d[1] / d[2]
                    out += [x / efl, (y - yc) / efl]
    return np.array(out)


def levenberg(surfs, fnum, half_field, extra=None, iters=40, gaps=True):
    """Levenberg-Marquardt on the curvatures (and air gaps), rescaling to
    f = 100; extra(surfs) returns more residuals (constraints)."""
    free_c = [i for i, s in enumerate(surfs) if s[0] != 0.0]
    free_d = [i for i, s in enumerate(surfs) if gaps and s[2] is None and i < len(surfs) - 1]
    def build(x):
        trial = [list(s) for s in surfs]
        for k, i in enumerate(free_c):
            trial[i][0] = 1.0 / x[k] if abs(x[k]) > 1e-9 else 1e9
        for k, i in enumerate(free_d):
            trial[i][1] = max(0.05, x[len(free_c) + k])
        return scaled(trial, 100.0)
    def resid(x):
        try:
            t = build(x)
            r = residuals(t, fnum, half_field)
            if r is None:
                return None
            if extra:
                r = np.concatenate([r, extra(t)])
            return r
        except Exception:
            return None
    x = np.array([1.0 / surfs[i][0] for i in free_c] + [surfs[i][1] for i in free_d])
    r = resid(x)
    lam = 1e-3
    for it in range(iters):
        J = np.zeros((len(r), len(x)))
        for k in range(len(x)):
            h = 1e-6 * max(1.0, abs(x[k])) if k >= len(free_c) else 1e-7
            xp = x.copy()
            xp[k] += h
            rp = resid(xp)
            if rp is None or len(rp) != len(r):
                continue
            J[:, k] = (rp - r) / h
        g = J.T @ r
        A = J.T @ J
        for _ in range(10):
            step = np.linalg.solve(A + lam * np.diag(np.diag(A) + 1e-12), -g)
            rn = resid(x + step)
            if rn is not None and len(rn) == len(r) and rn @ rn < r @ r:
                x, r = x + step, rn
                lam = max(lam / 3.0, 1e-9)
                break
            lam *= 4.0
        print(f"  LM {it}: rms {math.sqrt(r @ r / len(r)) * 100:.5f} mm at f=100", file=sys.stderr)
    return build(x)


LENSES = [
    # name, description, surfaces at f=100, nominal focal length, f-number,
    # half-diagonal of the 55 mm square, status
    ("biogon-38mm", "Zeiss Biogon 38 mm f/4.5 (US 2,721,499 Ex. 2, Bertele): the SWC's lens",
     BIOGON, 38.0, 4.5, "patent example"),
    ("biogon-60mm", "Zeiss Biogon 60 mm f/5.6 (HDC): a reconstruction from the Biogon of US 2,721,499",
     BIOGON, 61.1, 5.6, "reconstruction"),
    ("biogon-60mm-reseau", "Zeiss Biogon 60 mm f/5.6 with the Hasselblad Data Camera's Reseau plate (3 mm, assumed BK7)",
     "reseau", 61.1, 5.6, "reconstruction"),
    ("planar-80mm", "Zeiss Planar 80 mm f/2.8 (US 2,724,994 Ex. I, Lange): 5 elements, 4 groups",
     PLANAR_I, 80.0, 2.8, "patent example"),
    ("planar-100mm", "Zeiss Planar 100 mm f/3.5: the Planar of US 2,724,994 Ex. II at 100 mm and f/3.5",
     PLANAR_II, 100.0, 3.5, "patent family"),
    ("uv-sonnar-105mm", "Zeiss UV-Sonnar 105 mm f/4.3: a reconstruction, 7 air-spaced silica and CaF2 singlets",
     UVSONNAR, 105.0, 4.3, "reconstruction"),
    ("sonnar-250mm", "Zeiss Sonnar 250 mm f/5.6: a reconstruction, 4 elements in 3 groups",
     SONNAR, 250.0, 5.6, "reconstruction"),
    ("tele-tessar-500mm", "Zeiss Tele-Tessar 500 mm f/8: a reconstruction, 5 elements in 3 groups",
     TELETESSAR, 500.0, 8.0, "reconstruction"),
]


def half_field(focal):
    return math.atan(55.0 * math.sqrt(2.0) / 2.0 / focal)


RESEAU_PLATE = (3.0, (1.51680, 64.17))      # thickness (mm) and glass: assumptions


def report(name, surfs, focal, fnum):
    if surfs == "reseau":
        s = with_plate(scaled(BIOGON, focal), RESEAU_PLATE[0], RESEAU_PLATE[1])
    else:
        s = scaled(surfs, focal)
    efl, bfd = paraxial(s)
    hf = half_field(focal)
    ap, sr = apertures(s, fnum, hf)
    zimg = vertices(s)[-1] + bfd
    dist = []
    for fr in (0.5, 1.0):
        h = image_height(s, hf * fr, zimg)
        ideal = efl * math.tan(hf * fr)
        dist.append(100.0 * (abs(h) - ideal) / ideal if h is not None else float("nan"))
    length = vertices(s)[-1]
    sp = spot(s, fnum, hf, strict=False)
    print(f"{name:20s} EFL {efl:8.3f}  BFD {bfd:7.2f}  length {length:7.2f}  stop r {sr:6.3f}"
          f"  distortion {dist[0]:+.2f}% / {dist[1]:+.2f}%  RMS spot {sp*1000:7.1f} um", file=sys.stderr)
    return s, ap


def table(name, s, ap):
    print(f"    const CLens::SSurface s_ao{name}[] =")
    print("    {")
    for surf, a in zip(s, ap):
        r, d, glass, isstop = surf
        if glass is None:
            n, v, g = 1.0, 0.0, "CLens::GLASS_NONE"
        elif isinstance(glass, str):
            n, v, g = sellmeier(glass, WL_D), 0.0, {"silica": "CLens::GLASS_FUSED_SILICA", "caf2": "CLens::GLASS_CALCIUM_FLUORIDE"}[glass]
        else:
            n, v, g = glass[0], glass[1], "CLens::GLASS_NONE"
        print(f"        {{ {r:10.4f}, {d:8.4f}, {n:.5f}, {a:7.3f}, {'TRUE ' if isstop else 'FALSE'}, {v:5.2f}, {g} }},")
    print("    };")


def with_plate(surfs, thickness, glass):
    """The lens with a flat glass plate whose back face sits at the focal
    plane (a Reseau plate pressed against the film)."""
    _, bfd = paraxial(surfs)
    n = glass[0]
    gap = bfd - thickness / n
    out = [list(s) for s in surfs]
    out[-1][1] = gap
    out.append(S(0.0, thickness, glass))
    out.append(S(0.0, 0.0))
    return out


def edge_and_track(fnum, half_field, track=None, bfd_min=None):
    """Constraint residuals: edge thickness of every element at least 0.3
    (at f = 100), the total track (front vertex to focus) at most track,
    and the back focus at least bfd_min."""
    def extra(s):
        out = []
        ap, _ = apertures(s, fnum, half_field)
        def sag(r, a):
            return 0.0 if r == 0 else r - math.copysign(math.sqrt(max(r * r - a * a, 0.0)), r)
        for i in range(len(s) - 1):
            a = max(ap[i], ap[i + 1])
            e = s[i][1] - sag(s[i][0], a) + sag(s[i + 1][0], a)
            out.append(max(0.0, 0.3 - e) * 0.01)
        _, bfd = paraxial(s)
        if track is not None:
            out.append(max(0.0, vertices(s)[-1] + bfd - track) * 0.002)
        if bfd_min is not None:
            out.append(max(0.0, bfd_min - bfd) * 0.002)
        return np.array(out)
    return extra


def main():
    global SONNAR, TELETESSAR, UVSONNAR
    if "--lm" in sys.argv:
        print("Sonnar 250:", file=sys.stderr)
        SONNAR = levenberg(SONNAR, 5.6, half_field(250.0), edge_and_track(5.6, half_field(250.0), 95.0, 30.0))
        print("Tele-Tessar 500:", file=sys.stderr)
        TELETESSAR = levenberg(TELETESSAR, 8.0, half_field(500.0), edge_and_track(8.0, half_field(500.0), 80.0, 15.0))
        print("UV-Sonnar 105:", file=sys.stderr)
        UVSONNAR = levenberg(UVSONNAR, 4.3, half_field(105.0), edge_and_track(4.3, half_field(105.0), None, 30.0))
        for nm, s in (("SONNAR", SONNAR), ("TELETESSAR", TELETESSAR), ("UVSONNAR", UVSONNAR)):
            print(f"{nm} = [")
            for r, d, g, st in s:
                print(f"    S({r:.4f}, {d:.4f}, {g!r}, {st}),")
            print("]")
        return
    if "--optimize" in sys.argv:
        bfd_min = lambda m: (lambda s: max(0.0, m - paraxial(s)[1]) * 0.05)
        print("Sonnar 250:", file=sys.stderr)
        SONNAR = optimize(SONNAR_START, 5.6, half_field(250.0), bfd_min(30.0))
        print("Tele-Tessar 500:", file=sys.stderr)
        TELETESSAR = optimize(TELETESSAR_START, 8.0, half_field(500.0), bfd_min(15.0))
        print("UV-Sonnar 105:", file=sys.stderr)
        UVSONNAR = optimize(UVSONNAR_START, 4.3, half_field(105.0), bfd_min(30.0))
        for nm, s in (("SONNAR", SONNAR), ("TELETESSAR", TELETESSAR), ("UVSONNAR", UVSONNAR)):
            print(f"{nm} = [")
            for r, d, g, st in s:
                print(f"    S({r:.4f}, {d:.4f}, {g!r}, {st}),")
            print("]")
        return
    if "--write" in sys.argv:
        # Splice the tables into Lens.cpp between its markers.
        import io
        import contextlib
        path = sys.argv[sys.argv.index("--write") + 1]
        buf = io.StringIO()
        sys.argv.append("--table")
        with contextlib.redirect_stdout(buf):
            main_tables()
        text = buf.getvalue()
        tables, entries = text.split("        // ENTRIES\n")
        src = open(path).read()
        def splice(src, begin, end, body):
            i = src.index(begin) + len(begin)
            j = src.index(end)
            return src[:i] + "\n" + body + src[j:]
        src = splice(src, "    // BEGIN APOLLO LENSES (tools/apollo_lenses.py --write)", "    // END APOLLO LENSES", tables)
        src = splice(src, "        // BEGIN APOLLO ENTRIES", "        // END APOLLO ENTRIES", entries)
        open(path, "w").write(src)
        return
    main_tables()


def main_tables():
    entries = []
    for name, desc, surfs, focal, fnum, status in LENSES:
        s, ap = report(name, surfs, focal, fnum)
        if "--table" in sys.argv:
            ident = "".join(w.capitalize() for w in name.replace("-", " ").split())
            print(f"        // {desc} ({status}).")
            table(ident, s, ap)
            print()
            entries.append(f'        {{ "{name}", "{desc}",\n'
                           f'          s_ao{ident}, (int)( sizeof( s_ao{ident} ) / sizeof( s_ao{ident}[ 0 ] ) ) }},')
    if entries:
        print("        // ENTRIES")
        print("\n".join(entries))


if __name__ == "__main__":
    main()
