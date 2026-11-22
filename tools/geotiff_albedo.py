#!/usr/bin/env python3
"""Albedo textures for Ray's terrain meshes, from LROC NAC orthoimages.

    geotiff_albedo.py ORTHO --dtm DTM --center LAT LON --extent METERS --size PIXELS --out OUT.png
                      [--sun AZIMUTH ELEVATION | --fit-sun] [--highpass METERS]
                      [--mask-lander METERS] [--report PREVIEW.png]

ORTHO is an orthoimage in the same equirectangular projection as the DTM (the
LROC NAC DTM releases come with them: NAC_DTM_APOLLO11_M150361817_50CM.IMG and
_2M.IMG for the Apollo 11 site), read as PDS3 (8- or 16-bit integer, or 32-bit
float) or GeoTIFF with tools/geotiff_terrain.py's readers; only the rows the
crop needs are read, so a sparse or partial download works.

The texture covers the square from -EXTENT to +EXTENT meters east and north of
the center, SIZE x SIZE pixels, north at the top: exactly what a terrain mesh
written by geotiff_terrain.py with --uv-extent EXTENT maps onto.

The orthoimage was taken in sunlight from its own direction, so every slope is
brighter or darker than its albedo. The tool removes that: it computes the
Lommel-Seeliger shading of the DTM under the orthoimage's sun (--sun, ground
azimuth from north and elevation in degrees; or --fit-sun, which finds the sun
that best explains the image's brightness from the DTM's slopes), divides the
image by it (pixels whose shading is under 0.3 of flat ground's, shadowed
slopes and DTM artifacts, are filled in from around them instead), and then by a broad Gaussian blur of the result
(--highpass, its sigma in meters; 0 for none), which takes out what slow
brightness changes remain and leaves the albedo mottling: ejecta halos, dark
patches and tracks. --mask-lander paints over the lander and its shadow within
that many meters of the center (the scene has its own lander there) by
smoothing in the pixels around them, keeping the bright ring the descent
engine swept. --destripe takes out north-south seams between the strips the
orthoimage was mosaicked from, and where the orthoimage has no data the
texture fades to the mean.

The output is an 8-bit grayscale PNG, linear (not sRGB), whose mean is 0.5
(128): the albedo relative to the mean, halved. In the scene a texture
multiplies it back to the regolith's albedo and color (see
scenes/terrain/README.md). Only the Python standard library, numpy and Pillow
are used.
"""
import argparse
import math
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import geotiff_terrain as gt  # noqa: E402

MOON_RADIUS = gt.MOON_RADIUS


class Lazy:
    """An image read row by row from a PDS3 product without loading it all."""

    def __init__(self, path):
        with open(path, 'rb') as f:
            head = f.read(65536).decode('latin-1')
        label = {}
        for line in head.splitlines():
            if line.strip() == 'END':
                break
            if '=' in line and not line.strip().startswith('/*'):
                key, _, value = line.partition('=')
                label.setdefault(key.strip(), value.split('/*')[0].strip())

        def num(key):
            return float(label[key].split('<')[0].strip().strip('"'))
        record = int(num('RECORD_BYTES'))
        start = int(label['^IMAGE'].split('<')[0].strip().strip('"').split(',')[-1].strip())
        lines, samples, bits = int(num('LINES')), int(num('LINE_SAMPLES')), int(num('SAMPLE_BITS'))
        kind = label['SAMPLE_TYPE']
        dtype = {('PC_REAL', 32): '<f4', ('IEEE_REAL', 32): '>f4', ('LSB_INTEGER', 16): '<i2',
                 ('MSB_INTEGER', 16): '>i2', ('UNSIGNED_INTEGER', 8): 'u1', ('LSB_UNSIGNED_INTEGER', 16): '<u2',
                 ('MSB_UNSIGNED_INTEGER', 16): '>u2'}[(kind, bits)]
        self.data = np.memmap(path, dtype=dtype, mode='r', offset=(start - 1) * record, shape=(lines, samples))
        self.null_below = -32760 if dtype in ('<i2', '>i2') else (-1e30 if 'f' in dtype else None)
        scale = num('MAP_SCALE') * (1000.0 if 'KM' in label['MAP_SCALE'] else 1.0)
        radius = num('A_AXIS_RADIUS') * 1000.0
        lat_ts, lon0 = math.radians(num('CENTER_LATITUDE')), num('CENTER_LONGITUDE')
        line_off, sample_off = num('LINE_PROJECTION_OFFSET'), num('SAMPLE_PROJECTION_OFFSET')
        self.scale = scale

        def to_pixel(lat, lon):
            dlon = ((lon - lon0 + 180.0) % 360.0) - 180.0
            return line_off - radius * np.radians(lat) / scale, sample_off + radius * np.radians(dlon) * math.cos(lat_ts) / scale
        self.to_pixel = to_pixel

    def sample(self, lat, lon):
        """Bilinear samples at arrays of lat/lon (NaN where there's no data)."""
        row, col = self.to_pixel(lat, lon)
        r0 = np.floor(row).astype(np.int64)
        c0 = np.floor(col).astype(np.int64)
        lo_r, hi_r = max(int(r0.min()), 0), min(int(r0.max()) + 2, self.data.shape[0])
        lo_c, hi_c = max(int(c0.min()), 0), min(int(c0.max()) + 2, self.data.shape[1])
        block = np.array(self.data[lo_r:hi_r, lo_c:hi_c], dtype=np.float64)
        if self.null_below is not None:
            block[block <= self.null_below] = np.nan
        block[block == 0] = np.nan if self.null_below is not None and self.null_below < -1e29 else block[block == 0]
        pad = np.pad(block, 1, constant_values=np.nan)
        rr = np.clip(r0 - lo_r + 1, 0, pad.shape[0] - 2)
        cc = np.clip(c0 - lo_c + 1, 0, pad.shape[1] - 2)
        fr, fc = row - r0, col - c0
        return (pad[rr, cc] * (1 - fr) * (1 - fc) + pad[rr, cc + 1] * (1 - fr) * fc +
                pad[rr + 1, cc] * fr * (1 - fc) + pad[rr + 1, cc + 1] * fr * fc)


class FromGrid:
    """geotiff_terrain's whole-image readers (GeoTIFFs and our crops) behind the same interface."""

    def __init__(self, path):
        self.grid = gt.read_any(path)
        self.scale = self.grid.scale

    def sample(self, lat, lon):
        flat_lat, flat_lon = lat.ravel(), lon.ravel()
        rows = np.empty(flat_lat.size)
        cols = np.empty(flat_lat.size)
        for i, (a, b) in enumerate(zip(flat_lat, flat_lon)):
            rows[i], cols[i] = self.grid.to_pixel(float(a), float(b))
        h = np.pad(self.grid.heights, 1, constant_values=np.nan)
        r0 = np.clip(np.floor(rows + 1).astype(int), 0, h.shape[0] - 2)
        c0 = np.clip(np.floor(cols + 1).astype(int), 0, h.shape[1] - 2)
        fr, fc = rows + 1 - r0, cols + 1 - c0
        v = (h[r0, c0] * (1 - fr) * (1 - fc) + h[r0, c0 + 1] * (1 - fr) * fc +
             h[r0 + 1, c0] * fr * (1 - fc) + h[r0 + 1, c0 + 1] * fr * fc)
        return v.reshape(lat.shape)


def opener(path):
    with open(path, 'rb') as f:
        magic = f.read(4)
    return FromGrid(path) if magic[:2] in (b'II', b'MM') else Lazy(path)


def grid_latlon(lat0, lon0, extent, size):
    step = 2.0 * extent / size
    coords = -extent + step * (np.arange(size) + 0.5)
    east = coords[None, :].repeat(size, axis=0)
    north = coords[::-1][:, None].repeat(size, axis=1)       # row 0 north
    lat = lat0 + np.degrees(north / MOON_RADIUS)
    lon = lon0 + np.degrees(east / (MOON_RADIUS * math.cos(math.radians(lat0))))
    return lat, lon, east, north, step


def shading(heights, step, azimuth, elevation):
    """Lommel-Seeliger brightness of the slopes under a sun at (azimuth, elevation),
    relative to flat ground, seen from straight above."""
    dz_dn, dz_de = np.gradient(heights, step)
    dz_dn = -dz_dn                       # rows run north to south
    n = np.stack([-dz_de, -dz_dn, np.ones_like(heights)], axis=-1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    az, el = math.radians(azimuth), math.radians(elevation)
    sun = np.array([math.sin(az) * math.cos(el), math.cos(az) * math.cos(el), math.sin(el)])
    mu0 = np.clip(n @ sun, 0.0, None)
    mu = n[..., 2]
    flat = math.sin(el) / (math.sin(el) + 1.0)
    return (mu0 / (mu0 + mu)) / flat


def gaussian(image, sigma):
    """A Gaussian blur by FFT, with the edges mirrored (NaNs filled with the mean first)."""
    filled = np.where(np.isnan(image), np.nanmean(image), image)
    big = np.pad(filled, [(filled.shape[0] // 2,) * 2, (filled.shape[1] // 2,) * 2], mode='reflect')
    fy = np.fft.fftfreq(big.shape[0])[:, None]
    fx = np.fft.rfftfreq(big.shape[1])[None, :]
    kernel = np.exp(-2.0 * (math.pi * sigma) ** 2 * (fx * fx + fy * fy))
    out = np.fft.irfft2(np.fft.rfft2(big) * kernel, s=big.shape)
    r, c = filled.shape[0] // 2, filled.shape[1] // 2
    return out[r:r + filled.shape[0], c:c + filled.shape[1]]


def fit_sun(image, heights, step, valid):
    """The sun (azimuth, elevation) whose shading best correlates with the image's brightness."""
    best = (-2.0, 0.0, 0.0)
    detail = image / gaussian(image, 40.0 / step)
    for elevation in range(10, 61, 5):
        for azimuth in range(0, 360, 5):
            s = shading(heights, step, azimuth, elevation)
            m = valid & np.isfinite(s)
            c = np.corrcoef(detail[m], s[m])[0, 1]
            if c > best[0]:
                best = (c, azimuth, elevation)
    _, a0, e0 = best
    for elevation in np.arange(e0 - 4, e0 + 4.1, 1.0):
        for azimuth in np.arange(a0 - 4, a0 + 4.1, 1.0):
            s = shading(heights, step, azimuth, elevation)
            m = valid & np.isfinite(s)
            c = np.corrcoef(detail[m], s[m])[0, 1]
            if c > best[0]:
                best = (c, float(azimuth), float(elevation))
    return best


def inpaint(image, mask, iterations=400):
    """Fills the masked pixels from their surroundings by repeated neighbour averaging."""
    out = image.copy()
    out[mask] = np.nanmean(image[~mask])
    for _ in range(iterations):
        pad = np.pad(out, 1, mode='edge')
        avg = (pad[:-2, 1:-1] + pad[2:, 1:-1] + pad[1:-1, :-2] + pad[1:-1, 2:]) / 4.0
        out[mask] = avg[mask]
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('ortho')
    ap.add_argument('--dtm', required=True)
    ap.add_argument('--center', nargs=2, type=float, required=True, metavar=('LAT', 'LON'))
    ap.add_argument('--extent', type=float, required=True)
    ap.add_argument('--size', type=int, required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--sun', nargs=2, type=float, metavar=('AZIMUTH', 'ELEVATION'))
    ap.add_argument('--fit-sun', action='store_true')
    ap.add_argument('--highpass', type=float, default=0.0, metavar='METERS')
    ap.add_argument('--mask-lander', type=float, default=0.0, metavar='METERS')
    ap.add_argument('--destripe', action='store_true', help='remove north-south seams between image strips')
    ap.add_argument('--report', help='write a strip: the orthoimage crop, the de-shaded albedo and the texture')
    args = ap.parse_args(argv)

    lat0, lon0 = args.center
    lat, lon, east, north, step = grid_latlon(lat0, lon0, args.extent, args.size)
    ortho = opener(args.ortho).sample(lat, lon)
    heights = opener(args.dtm).sample(lat, lon)
    valid = np.isfinite(ortho) & np.isfinite(heights) & (ortho > 0)
    print('%s: %d x %d at %.3f m, %.1f%% with data' % (args.ortho, args.size, args.size, step, 100.0 * valid.mean()))
    heights = np.where(np.isfinite(heights), heights, np.nanmean(heights))
    ortho_f = np.where(valid, ortho, np.nanmean(ortho[valid]))
    dist = np.hypot(east, north)
    lander = np.zeros_like(valid)
    if args.mask_lander > 0:
        lander = dist < args.mask_lander
    if args.fit_sun:
        corr, az, el = fit_sun(ortho_f, heights, step, valid & ~lander)
        print('fitted sun: azimuth %.1f, elevation %.1f (correlation %.3f)' % (az, el, corr))
    elif args.sun:
        az, el = args.sun
    else:
        raise SystemExit('give --sun or --fit-sun')
    raw_shade = shading(heights, step, az, el)
    shade = np.clip(raw_shade, 0.2, None)
    albedo = ortho_f / shade
    # Where the slopes turn nearly away from the sun (shadowed crater walls,
    # and spikes where the DTM's strips meet) the division means nothing:
    # fill those pixels from around them.
    dim = raw_shade < 0.3
    pad = np.pad(dim, 1)
    dim = dim | pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:]
    if dim.any():
        albedo = inpaint(albedo, dim, 60)
    print('%.2f%% of the pixels too dim to de-shade, filled in' % (100.0 * dim.mean()))
    if args.mask_lander > 0:
        # The lander is far brighter and its shadow far darker than any ground near it.
        local = albedo / gaussian(albedo, 6.0 / step)
        az_shadow = math.radians(az + 180.0)
        along = east * math.sin(az_shadow) + north * math.cos(az_shadow)
        across = np.abs(east * math.cos(az_shadow) - north * math.sin(az_shadow))
        shadow = (along > -1.0) & (along < 2.8 * args.mask_lander) & (across < 0.75 * args.mask_lander)
        odd = (local > 1.35) | (local < 0.7)
        lander = ((dist < 0.5 * args.mask_lander) | shadow | ((dist < args.mask_lander) & odd))
        for _ in range(max(1, int(round(1.0 / step)))):     # grow it a meter past the edges
            pad = np.pad(lander, 1)
            lander = lander | pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:]
        albedo = inpaint(albedo, lander)
    if args.highpass > 0:
        albedo = albedo / gaussian(albedo, args.highpass / step)
    ratio = albedo / np.mean(albedo[valid])
    if args.destripe:
        # The seams between NAC frames run north-south: take out what makes
        # one column brighter than the columns around it.
        log = np.where(valid, np.log(np.clip(ratio, 1e-3, None)), np.nan)
        counts = valid.sum(axis=0)
        profile = np.where(counts > 0, np.nansum(np.where(valid, log, 0.0), axis=0) / np.maximum(counts, 1), 0.0)
        width = max(2.0, 8.0 / step)
        x = np.arange(-int(4 * width), int(4 * width) + 1)
        kernel = np.exp(-0.5 * (x / width) ** 2)
        smooth = (np.convolve(np.pad(profile, len(x) // 2, mode='reflect'), kernel / kernel.sum(), mode='valid'))
        ratio = ratio * np.exp(-(profile - smooth))[None, :]
    ratio = np.where(valid, ratio, 1.0)
    if not valid.all():
        # Fade to the mean toward where the orthoimage has no data.
        edge = np.clip(1.25 * gaussian(valid.astype(np.float64), 40.0 / step) - 0.25, 0.0, 1.0)
        ratio = 1.0 + (ratio - 1.0) * edge * valid
    pixels = np.clip(np.round(ratio * 127.5), 0, 255).astype(np.uint8)
    Image.fromarray(pixels, mode='L').save(args.out, optimize=True)
    print('wrote %s: mean %.1f, 1st-99th percentile %d-%d, %.2f%% clipped' %
          (args.out, pixels.mean(), np.percentile(pixels, 1), np.percentile(pixels, 99),
           100.0 * ((pixels == 0) | (pixels == 255)).mean()))
    if args.report:
        def norm(a):
            lo, hi = np.percentile(a[valid], [1, 99])
            return np.clip((a - lo) / (hi - lo) * 255, 0, 255).astype(np.uint8)
        strip = np.concatenate([norm(ortho_f), norm(albedo), pixels], axis=1)
        Image.fromarray(strip, mode='L').save(args.report)
        print('wrote', args.report)


if __name__ == '__main__':
    main()
