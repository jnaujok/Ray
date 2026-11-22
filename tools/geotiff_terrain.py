#!/usr/bin/env python3
"""Terrain meshes from lunar elevation models, for Ray.

    geotiff_terrain.py INPUT --center LAT LON --radius METERS --out MESH.glb
                       [--inner METERS] [--step N] [--curvature] [--crop OUT.tif]
                       [--sample LAT LON ...] [--uv-extent METERS]

INPUT is a 32-bit float elevation model: a GeoTIFF (uncompressed, strips or
tiles, float32 or int16, one band) or a PDS3 image with an attached label (the
LROC NAC DTMs are distributed as PDS3 .IMG files; their .TIF files are 8-bit
browse images, not heights). Both must be in an equirectangular (simple
cylindrical) projection of the Moon, as LROC and USGS products are; the
projection's center longitude and true-scale latitude come from the PDS label,
or from the GeoTIFF's GeoKeys (ProjCenterLongGeoKey, ProjStdParallel1GeoKey).

The mesh is in Ray's scene axes with its origin on the ground at --center:
x east, y up, z north, in meters, heights relative to the elevation at the
center. Every grid point within --radius (and outside --inner, so a coarse
outer ring can surround a fine inner patch) becomes a vertex, taking every
--step'th point, and each grid cell whose four corners are in becomes two
triangles. With --curvature, points drop by d^2 / 2R for their distance d, as
the Moon's surface curves away (at 1.4 m eye height the horizon is about
2.2 km off). The output is a binary glTF (.glb) with positions, smooth
normals and indices, which Ray loads as a mesh. --uv-extent METERS adds
texture coordinates that map the square from -METERS to +METERS east and
north of the center onto an image, north at the top, as the albedo textures
tools/geotiff_albedo.py writes.

--crop writes the cropped heights as a small float32 GeoTIFF in the same
local frame (meters east and north of the center), so a crop can be kept in
place of a 100+ MB product; the tool reads its own crops back. --sample prints
the height at points (relative to the center), e.g. where a camera stood.

Only the Python standard library and numpy are used.
"""
import argparse
import json
import math
import struct
import sys

import numpy as np

MOON_RADIUS = 1737400.0

# ---------------------------------------------------------------------------
# Readers. Each returns (heights[lines, samples] float64 with NaN for missing,
# a function (lat, lon) -> (line, sample), a function (line, sample) -> (x east m, y north m) about a center).


class Grid:
    def __init__(self, heights, scale, to_pixel):
        self.heights = heights          # [row, column], row 0 north
        self.scale = scale              # meters per pixel
        self.to_pixel = to_pixel        # (lat, lon) -> (row, column), floats


def read_pds3(path):
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
    if label.get('MAP_PROJECTION_TYPE', '').strip('"') not in ('EQUIRECTANGULAR', 'SIMPLE CYLINDRICAL'):
        raise SystemExit('%s: only equirectangular projections are supported' % path)
    dtype = {('PC_REAL', 32): '<f4', ('IEEE_REAL', 32): '>f4', ('LSB_INTEGER', 16): '<i2', ('MSB_INTEGER', 16): '>i2'}
    if (kind, bits) not in dtype:
        raise SystemExit('%s: unsupported sample type %s %d' % (path, kind, bits))
    data = np.fromfile(path, dtype=dtype[(kind, bits)], count=lines * samples, offset=(start - 1) * record)
    heights = data.reshape(lines, samples).astype(np.float64)
    heights[heights < -1e30] = np.nan
    if 'SCALING_FACTOR' in label:
        heights = heights * num('SCALING_FACTOR') + (num('OFFSET') if 'OFFSET' in label else 0.0)
    scale = num('MAP_SCALE') * (1000.0 if 'KM' in label['MAP_SCALE'] else 1.0)
    radius = num('A_AXIS_RADIUS') * 1000.0
    lat_ts, lon0 = math.radians(num('CENTER_LATITUDE')), num('CENTER_LONGITUDE')
    line_off, sample_off = num('LINE_PROJECTION_OFFSET'), num('SAMPLE_PROJECTION_OFFSET')

    def to_pixel(lat, lon):
        dlon = ((lon - lon0 + 180.0) % 360.0) - 180.0
        x = radius * math.radians(dlon) * math.cos(lat_ts)
        y = radius * math.radians(lat)
        # PDS: sample = x / scale + SAMPLE_PROJECTION_OFFSET + 1, 1-based; 0-based here.
        return line_off - y / scale, sample_off + x / scale

    return Grid(heights, scale, to_pixel)


TIFF_TYPES = {1: ('B', 1), 2: ('s', 1), 3: ('H', 2), 4: ('I', 4), 11: ('f', 4), 12: ('d', 8), 16: ('Q', 8)}


def read_tiff(path):
    with open(path, 'rb') as f:
        data = f.read()
    order = {b'II': '<', b'MM': '>'}.get(data[:2])
    if order is None or struct.unpack(order + 'H', data[2:4])[0] != 42:
        raise SystemExit('%s: not a (classic) TIFF file' % path)
    offset = struct.unpack(order + 'I', data[4:8])[0]
    count = struct.unpack(order + 'H', data[offset:offset + 2])[0]
    tags = {}
    for i in range(count):
        tag, kind, n, value = struct.unpack(order + 'HHI4s', data[offset + 2 + 12 * i:offset + 14 + 12 * i])
        fmt, size = TIFF_TYPES.get(kind, ('B', 1))
        raw = value if n * size <= 4 else data[struct.unpack(order + 'I', value)[0]:struct.unpack(order + 'I', value)[0] + n * size]
        tags[tag] = raw[:n] if fmt == 's' else list(struct.unpack(order + fmt * n, raw[:n * size]))
    width, height = tags[256][0], tags[257][0]
    bits, compression = tags.get(258, [8])[0], tags.get(259, [1])[0]
    sample_format = tags.get(339, [1])[0]
    if compression != 1:
        raise SystemExit('%s: compressed TIFFs are not supported (compression %d)' % (path, compression))
    if tags.get(277, [1])[0] != 1:
        raise SystemExit('%s: only one-band elevation models are supported' % path)
    dtype = {(3, 32): 'f4', (3, 64): 'f8', (2, 16): 'i2', (1, 16): 'u2', (2, 32): 'i4'}.get((sample_format, bits))
    if dtype is None:
        raise SystemExit('%s: %d-bit %s samples are not elevations (use the float32 product, not a browse image)'
                         % (path, bits, {1: 'unsigned', 2: 'signed', 3: 'float'}.get(sample_format, '?')))
    dt = np.dtype(order + dtype)
    heights = np.zeros((height, width), dtype=np.float64)
    if 322 in tags:     # tiles
        tw, th = tags[322][0], tags[323][0]
        across = (width + tw - 1) // tw
        for index, (start, nbytes) in enumerate(zip(tags[324], tags[325])):
            tile = np.frombuffer(data[start:start + nbytes], dtype=dt, count=tw * th).reshape(th, tw)
            r, c = (index // across) * th, (index % across) * tw
            heights[r:r + th, c:c + tw] = tile[:min(th, height - r), :min(tw, width - c)]
    else:               # strips
        rows = tags.get(278, [height])[0]
        for index, (start, nbytes) in enumerate(zip(tags[273], tags[279])):
            n = min(rows, height - index * rows)
            heights[index * rows:index * rows + n] = np.frombuffer(data[start:start + nbytes], dtype=dt, count=n * width).reshape(n, width)
    if 42113 in tags:   # GDAL_NODATA
        try:
            heights[heights == float(bytes(tags[42113]).decode().strip('\x00 '))] = np.nan
        except ValueError:
            pass
    heights[heights < -1e30] = np.nan
    scale_x, scale_y = tags[33550][0], tags[33550][1]
    tie = tags[33922]            # i, j, k, x, y, z
    keys = {}
    if 34735 in tags:
        g = tags[34735]
        doubles = tags.get(34736, [])
        for k in range(g[3]):
            key, loc, n, val = g[4 + 4 * k:8 + 4 * k]
            keys[key] = doubles[val] if loc == 34736 else val
    description = bytes(tags[270]).decode('latin-1').rstrip('\x00') if 270 in tags else ''
    if description.startswith('ray-local-terrain'):
        # Our own crop: model coordinates are meters east/north of the center.
        info = json.loads(description.split(' ', 1)[1])
        lat0, lon0 = info['lat'], info['lon']

        def to_pixel(lat, lon):
            x = MOON_RADIUS * math.radians(lon - lon0) * math.cos(math.radians(lat0))
            y = MOON_RADIUS * math.radians(lat - lat0)
            return (tie[4] - y) / scale_y + tie[1], (x - tie[3]) / scale_x + tie[0]
        return Grid(heights, scale_x, to_pixel)
    if keys.get(1024) == 2:
        # Geographic: model coordinates are degrees of longitude and latitude.
        radius = keys.get(2057, MOON_RADIUS)

        def to_pixel(lat, lon):
            return (tie[4] - lat) / scale_y + tie[1], (lon - tie[3]) / scale_x + tie[0]
        return Grid(heights, radius * math.radians(scale_x), to_pixel)
    lon0 = keys.get(3088, keys.get(3080, 0.0))
    lat_ts = math.radians(keys.get(3078, keys.get(3081, 0.0)))
    radius = keys.get(2057, MOON_RADIUS)

    def to_pixel(lat, lon):
        dlon = ((lon - lon0 + 180.0) % 360.0) - 180.0
        x = radius * math.radians(dlon) * math.cos(lat_ts)
        y = radius * math.radians(lat)
        return (tie[4] - y) / scale_y + tie[1], (x - tie[3]) / scale_x + tie[0]
    return Grid(heights, scale_x, to_pixel)


def read_any(path):
    with open(path, 'rb') as f:
        magic = f.read(4)
    return read_tiff(path) if magic[:2] in (b'II', b'MM') else read_pds3(path)


# ---------------------------------------------------------------------------

def bilinear(heights, row, col):
    r0, c0 = int(math.floor(row)), int(math.floor(col))
    fr, fc = row - r0, col - c0
    v = heights[r0:r0 + 2, c0:c0 + 2]
    return (v[0, 0] * (1 - fr) * (1 - fc) + v[0, 1] * (1 - fr) * fc + v[1, 0] * fr * (1 - fc) + v[1, 1] * fr * fc)


def local_grid(grid, lat, lon, radius, step):
    """Heights on a square grid in meters east (x) and north (z) of (lat, lon), spacing step pixels."""
    row0, col0 = grid.to_pixel(lat, lon)
    span = int(math.ceil(radius / (grid.scale * step))) + 1
    rows = row0 + np.arange(-span, span + 1) * step       # north to south
    cols = col0 + np.arange(-span, span + 1) * step       # west to east
    # Points off the model's edge are missing (NaN): the mesh stops where the data does.
    h = np.pad(grid.heights, 1, constant_values=np.nan)
    rows_p, cols_p = rows + 1.0, cols + 1.0
    r0 = np.clip(np.floor(rows_p).astype(int), 0, h.shape[0] - 2)
    c0 = np.clip(np.floor(cols_p).astype(int), 0, h.shape[1] - 2)
    fr = np.clip(rows_p - r0, 0.0, 1.0)[:, None]
    fc = np.clip(cols_p - c0, 0.0, 1.0)[None, :]
    z = (h[np.ix_(r0, c0)] * (1 - fr) * (1 - fc) + h[np.ix_(r0, c0 + 1)] * (1 - fr) * fc +
         h[np.ix_(r0 + 1, c0)] * fr * (1 - fc) + h[np.ix_(r0 + 1, c0 + 1)] * fr * fc)
    off = (rows < 0) | (rows > grid.heights.shape[0] - 1)
    z[off, :] = np.nan
    off = (cols < 0) | (cols > grid.heights.shape[1] - 1)
    z[:, off] = np.nan
    h = grid.heights
    center = bilinear(h, row0, col0)
    east = (cols - col0) * grid.scale
    north = (row0 - rows) * grid.scale
    return east, north, z - center, center


def fill_center(east, north, heights, radius):
    """Replace heights within radius of the center by a plane fitted to the ring from radius to
    2 radius: the stereo models that make DTMs see the lander itself as a bump in the ground."""
    E, N = np.meshgrid(east, north)
    d2 = E * E + N * N
    ring = (d2 >= radius * radius) & (d2 <= 4.0 * radius * radius) & ~np.isnan(heights)
    A = np.stack([np.ones(int(ring.sum())), E[ring], N[ring]], axis=1)
    (c0, ce, cn), *_ = np.linalg.lstsq(A, heights[ring], rcond=None)
    out = heights.copy()
    inside = d2 < radius * radius
    out[inside] = c0 + ce * E[inside] + cn * N[inside]
    return out - c0, (ce, cn, c0)


def tessellate(east, north, heights, radius, inner, curvature):
    E, N = np.meshgrid(east, north)
    d2 = E * E + N * N
    inside = (d2 <= radius * radius) & ~np.isnan(heights)
    if inner > 0:
        # Keep the cells that straddle the inner edge so the rings overlap a little.
        inside &= d2 >= (inner - 2.0 * abs(east[1] - east[0])) ** 2
    y = heights - (d2 / (2.0 * MOON_RADIUS) if curvature else 0.0)
    index = -np.ones(heights.shape, dtype=np.int64)
    index[inside] = np.arange(int(inside.sum()))
    positions = np.stack([E[inside], y[inside], N[inside]], axis=1)
    a, b, c, d = index[:-1, :-1], index[:-1, 1:], index[1:, :-1], index[1:, 1:]
    full = (a >= 0) & (b >= 0) & (c >= 0) & (d >= 0)
    # Rows run north to south and columns west to east; with y up, (a, c, b) and
    # (b, c, d) wind so the faces point up in Ray's left-handed axes.
    tris = np.concatenate([np.stack([a[full], c[full], b[full]], axis=1), np.stack([b[full], c[full], d[full]], axis=1)])
    normals = np.zeros_like(positions)
    p0, p1, p2 = positions[tris[:, 0]], positions[tris[:, 1]], positions[tris[:, 2]]
    face = np.cross(p1 - p0, p2 - p0)
    if face[:, 1].mean() < 0:
        tris = tris[:, [0, 2, 1]]
        face = -face
    for k in range(3):
        np.add.at(normals, tris[:, k], face)
    normals /= np.linalg.norm(normals, axis=1, keepdims=True)
    return positions.astype(np.float32), normals.astype(np.float32), tris.astype(np.uint32)


def write_glb(path, positions, normals, tris, name, uv_extent=0.0):
    bin_pos, bin_nrm, bin_idx = positions.tobytes(), normals.tobytes(), tris.reshape(-1).tobytes()
    pad = lambda b: b + b'\0' * ((4 - len(b) % 4) % 4)
    buffer = pad(bin_pos) + pad(bin_nrm) + pad(bin_idx)
    o_nrm = len(pad(bin_pos))
    o_idx = o_nrm + len(pad(bin_nrm))
    attributes = {"POSITION": 0, "NORMAL": 1}
    doc = {
        "asset": {"version": "2.0", "generator": "Ray tools/geotiff_terrain.py"},
        "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"name": name, "mesh": 0}],
        "meshes": [{"name": name, "primitives": [{"attributes": attributes, "indices": 2, "material": 0}]}],
        "materials": [{"name": "regolith", "pbrMetallicRoughness": {"baseColorFactor": [0.12, 0.115, 0.11, 1.0]}}],
        "buffers": [{"byteLength": len(buffer)}],
        "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": len(bin_pos), "target": 34962},
                        {"buffer": 0, "byteOffset": o_nrm, "byteLength": len(bin_nrm), "target": 34962},
                        {"buffer": 0, "byteOffset": o_idx, "byteLength": len(bin_idx), "target": 34963}],
        "accessors": [{"bufferView": 0, "componentType": 5126, "count": len(positions), "type": "VEC3",
                       "min": positions.min(axis=0).tolist(), "max": positions.max(axis=0).tolist()},
                      {"bufferView": 1, "componentType": 5126, "count": len(normals), "type": "VEC3"},
                      {"bufferView": 2, "componentType": 5125, "count": int(tris.size), "type": "SCALAR"}],
    }
    if uv_extent > 0:
        # u runs west to east and v north to south (glTF's v = 0 is an
        # image's top row) across the square +-uv_extent about the center,
        # so an image of that square, north up, lands on the ground 1:1.
        uv = np.stack([(positions[:, 0] + uv_extent) / (2.0 * uv_extent),
                       (uv_extent - positions[:, 2]) / (2.0 * uv_extent)], axis=1).astype(np.float32)
        bin_uv = uv.tobytes()
        doc["bufferViews"].append({"buffer": 0, "byteOffset": len(buffer), "byteLength": len(bin_uv), "target": 34962})
        doc["accessors"].append({"bufferView": 3, "componentType": 5126, "count": len(uv), "type": "VEC2"})
        attributes["TEXCOORD_0"] = 3
        buffer += pad(bin_uv)
        doc["buffers"][0]["byteLength"] = len(buffer)
    js = json.dumps(doc, separators=(',', ':')).encode()
    js += b' ' * ((4 - len(js) % 4) % 4)
    with open(path, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(buffer)))
        f.write(struct.pack('<II', len(js), 0x4E4F534A) + js)
        f.write(struct.pack('<II', len(buffer), 0x004E4942) + buffer)


def write_crop_tiff(path, heights, east, north, lat, lon, center):
    """The crop as a float32 GeoTIFF in meters east/north of (lat, lon), heights absolute."""
    rows, cols = heights.shape
    scale = float(east[1] - east[0])
    data = (heights + center).astype('<f4')
    data = np.where(np.isnan(data), np.float32(-3.4028226550889045e38), data).astype('<f4')
    desc = ('ray-local-terrain ' + json.dumps({"lat": lat, "lon": lon})).encode() + b'\0'
    pixel = struct.pack('<3d', scale, scale, 0.0)
    tie = struct.pack('<6d', 0.0, 0.0, 0.0, float(east[0]), float(north[0]), 0.0)
    image = data.tobytes()
    entries = []
    blobs = b''
    base = 8
    # Layout: header, image, blobs, IFD.
    blob_start = base + len(image)

    def blob(raw):
        nonlocal blobs
        off = blob_start + len(blobs)
        blobs += raw + (b'\0' if len(raw) % 2 else b'')
        return off
    entries.append((256, 3, 1, struct.pack('<HH', cols, 0)))
    entries.append((257, 3, 1, struct.pack('<HH', rows, 0)))
    entries.append((258, 3, 1, struct.pack('<HH', 32, 0)))
    entries.append((259, 3, 1, struct.pack('<HH', 1, 0)))
    entries.append((262, 3, 1, struct.pack('<HH', 1, 0)))
    entries.append((270, 2, len(desc), struct.pack('<I', blob(desc))))
    entries.append((273, 4, 1, struct.pack('<I', base)))
    entries.append((277, 3, 1, struct.pack('<HH', 1, 0)))
    entries.append((278, 3, 1, struct.pack('<HH', rows, 0)))
    entries.append((279, 4, 1, struct.pack('<I', len(image))))
    entries.append((339, 3, 1, struct.pack('<HH', 3, 0)))
    entries.append((33550, 12, 3, struct.pack('<I', blob(pixel))))
    entries.append((33922, 12, 6, struct.pack('<I', blob(tie))))
    ifd_at = blob_start + len(blobs)
    ifd = struct.pack('<H', len(entries)) + b''.join(struct.pack('<HHI', t, k, n) + v for t, k, n, v in entries) + struct.pack('<I', 0)
    with open(path, 'wb') as f:
        f.write(b'II' + struct.pack('<HI', 42, ifd_at) + image + blobs + ifd)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('input')
    ap.add_argument('--center', nargs=2, type=float, required=True, metavar=('LAT', 'LON'))
    ap.add_argument('--radius', type=float, required=True)
    ap.add_argument('--inner', type=float, default=0.0)
    ap.add_argument('--step', type=int, default=1)
    ap.add_argument('--curvature', action='store_true')
    ap.add_argument('--out')
    ap.add_argument('--crop')
    ap.add_argument('--sample', nargs=2, type=float, action='append', default=[], metavar=('LAT', 'LON'))
    ap.add_argument('--sample-local', nargs=2, type=float, action='append', default=[], metavar=('EAST', 'NORTH'))
    ap.add_argument('--fill-center', type=float, default=0.0, metavar='METERS',
                    help='replace the heights within this distance of the center by a plane fitted around it')
    ap.add_argument('--name', default='terrain')
    ap.add_argument('--uv-extent', type=float, default=0.0, metavar='METERS',
                    help='write texture coordinates mapping the square +-METERS about the center onto an image')
    args = ap.parse_args(argv)

    grid = read_any(args.input)
    lat, lon = args.center
    east, north, heights, center = local_grid(grid, lat, lon, args.radius, args.step)
    print('%s: %d x %d grid points, %.3g m apart, center elevation %.2f m' %
          (args.input, heights.shape[1], heights.shape[0], grid.scale * args.step, center))
    zero = 0.0      # how far the heights (relative to the DTM at the center) sit above the ground under it
    if args.fill_center > 0:
        # Fit the plane on the full-resolution grid; every grid shares its zero.
        fe, fn, fh, _ = local_grid(grid, lat, lon, 2.0 * args.fill_center + 2.0 * grid.scale, 1)
        _, (ce, cn, c0) = fill_center(fe, fn, fh, args.fill_center)
        zero = c0
        E, N = np.meshgrid(east, north)
        inside = E * E + N * N < args.fill_center ** 2
        heights = heights - c0
        heights[inside] = ce * E[inside] + cn * N[inside]
        slope = math.degrees(math.atan(math.hypot(ce, cn)))
        downhill = math.degrees(math.atan2(-ce, -cn)) % 360.0
        print('ground under the center: %.2f m below the DTM there (the lander bump), a plane sloping %.2f degrees, '
              'downhill toward azimuth %.1f' % (-c0, slope, downhill))
    for slat, slon in args.sample:
        r, c = grid.to_pixel(slat, slon)
        print('height at %.6f, %.6f: %+.3f m' % (slat, slon, bilinear(grid.heights, r, c) - center - zero))
    for e, n in args.sample_local:
        dlat = math.degrees(n / MOON_RADIUS)
        dlon = math.degrees(e / (MOON_RADIUS * math.cos(math.radians(lat))))
        r, c = grid.to_pixel(lat + dlat, lon + dlon)
        print('height at %.2f m east, %.2f m north: %+.3f m' % (e, n, bilinear(grid.heights, r, c) - center - zero))
    if args.crop:
        write_crop_tiff(args.crop, heights, east, north, lat, lon, center + zero)
        print('wrote %s' % args.crop)
    if args.out:
        positions, normals, tris = tessellate(east, north, heights, args.radius, args.inner, args.curvature)
        write_glb(args.out, positions, normals, tris, args.name, args.uv_extent)
        print('wrote %s: %d vertices, %d triangles' % (args.out, len(positions), len(tris)))


if __name__ == '__main__':
    main()
