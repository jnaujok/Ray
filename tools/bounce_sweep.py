#!/usr/bin/env python3
"""Measure indirect light's noise and cost against its number of rays.

    bounce_sweep.py RAYTHROWER [--scene scenes/bounce_test.ray] [--out DIR]
                    [--threads N] [--image PNG]

Renders the scene (bounce_test by default) with the "objects" method at 1, 4,
9, 16, 25, 36, 49 and 64 rays an object (--counts for others), and with
the "hemisphere" method at the same number of rays a point for a few of them
(the scene's objects in reach times the rays an object), timing each render
(wall and CPU time of raythrower) and measuring its noise: the root mean
square difference, on a 0-255 scale, from a reference render with many more
rays: the "objects" method at 256 rays an object, with the scene's own camera
sampler (both methods estimate the same light, so both are measured against
it). The noise is measured over the whole image
and over the indirect-lit region: the pixels a render without indirect light
leaves dark (under a tenth of the brightest), which only bounced light lights.

Prints a Markdown table and, with --image, writes a labelled grid of the
"objects" renders (needs Pillow). The reference renders are kept in --out, so
a second run only renders what changed. Nothing here is random: raythrower
gives the same image for the same scene every time.
"""
import argparse
import json
import os
import resource
import struct
import subprocess
import sys
import time

COUNTS = [1, 4, 9, 16, 25, 36, 49, 64]
REFERENCE_COUNT = 256
HEMISPHERE_FOR = [1, 4, 9, 16]


def read_raw(path):
    with open(path, 'rb') as f:
        data = f.read()
    width, height = struct.unpack('>HH', data[:4])
    return width, height, data[4:4 + width * height * 3]


def render(raythrower, scene, settings, output, threads, size, aa=None):
    """Renders scene with its settings.indirect replaced; returns (wall, cpu)."""
    with open(scene) as f:
        doc = json.load(f)
    if settings is None:
        doc['settings'].pop('indirect', None)
    else:
        doc['settings']['indirect'] = settings
    # The copy goes beside the scene, so the files it names (textures, the
    # sky, models) are found where the scene finds them.
    scene_copy = os.path.join(os.path.dirname(os.path.abspath(scene)),
                              '.bounce_sweep_' + os.path.basename(output)[:-4] + '.ray')
    with open(scene_copy, 'w') as f:
        json.dump(doc, f, indent=2)
    args = [raythrower, scene_copy, '-q', '-o', output, '-N%d' % threads]
    if size:
        args += ['--size', size]
    if aa:
        args += ['--aa', aa]
    before = resource.getrusage(resource.RUSAGE_CHILDREN)
    start = time.perf_counter()
    try:
        subprocess.run(args, check=True)
    finally:
        os.remove(scene_copy)
    wall = time.perf_counter() - start
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    cpu = (after.ru_utime - before.ru_utime) + (after.ru_stime - before.ru_stime)
    return wall, cpu


def rms(pixels, reference, mask=None):
    total = 0.0
    count = 0
    for index in range(0, len(pixels), 3):
        if mask is not None and not mask[index // 3]:
            continue
        for channel in range(3):
            difference = pixels[index + channel] - reference[index + channel]
            total += difference * difference
        count += 3
    return (total / count) ** 0.5 if count else 0.0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('raythrower')
    parser.add_argument('--scene', default=os.path.join(os.path.dirname(__file__), '..', 'scenes', 'bounce_test.ray'))
    parser.add_argument('--out', default='bounce_sweep')
    parser.add_argument('--threads', type=int, default=os.cpu_count() or 4)
    parser.add_argument('--size', default=None, help='image size, instead of the scene\'s')
    parser.add_argument('--image', default=None, help='write a labelled grid of the renders here')
    parser.add_argument('--counts', default=','.join(str(c) for c in COUNTS),
                        help='rays an object to measure, comma-separated (default %(default)s)')
    parser.add_argument('--hemisphere', default=','.join(str(c) for c in HEMISPHERE_FOR),
                        help='the counts also measured with the hemisphere method, at the same rays a point')
    parser.add_argument('--reference', type=int, default=REFERENCE_COUNT,
                        help='rays an object for the reference render (default %(default)s, at most 256)')
    options = parser.parse_args()

    os.makedirs(options.out, exist_ok=True)
    with open(options.scene) as f:
        base = json.load(f)['settings'].get('indirect', {})
    objects = dict(base, method='objects')
    objects.pop('samples', None)
    hemisphere = {k: v for k, v in base.items() if k not in ('objects', 'samples')}
    hemisphere['method'] = 'hemisphere'
    # The objects a point gathers from: every object in the scene but the
    # one it is on, up to the "objects" cap.
    with open(options.scene) as f:
        in_reach = min(len(json.load(f)['objects']) - 1, objects.get('objects', 8))

    def path(name):
        return os.path.join(options.out, name + '.raw')

    def ensure(name, settings, aa=None):
        if not os.path.exists(path(name)):
            print('rendering %s ...' % name, file=sys.stderr)
            render(options.raythrower, options.scene, settings, path(name), options.threads, options.size, aa)
        return read_raw(path(name))

    width, height, direct = ensure('direct', None)
    brightest = max(direct) or 1
    mask = [max(direct[i:i + 3]) < brightest / 10 for i in range(0, len(direct), 3)]
    counts = [int(c) for c in options.counts.split(',') if c]
    hemisphere_for = [int(c) for c in options.hemisphere.split(',') if c]
    _, _, objects_reference = ensure('objects_reference', dict(objects, samples=options.reference))
    # Both methods estimate the same light (format_test checks they agree),
    # so both are measured against the one reference, rendered with the
    # same camera sampler as the renders measured: a reference with a finer
    # sampler would differ at every edge and hide the noise.
    hemisphere_reference = objects_reference

    rows = []
    tiles = []
    for count in counts:
        name = 'objects_%d' % count
        wall, cpu = render(options.raythrower, options.scene, dict(objects, samples=count), path(name),
                           options.threads, options.size)
        _, _, pixels = read_raw(path(name))
        rows.append(('objects', count, count * in_reach, wall, cpu, rms(pixels, objects_reference),
                     rms(pixels, objects_reference, mask)))
        tiles.append((name, count, rows[-1][5], rows[-1][6]))
    for count in hemisphere_for:
        name = 'hemisphere_%d' % (count * in_reach)
        wall, cpu = render(options.raythrower, options.scene, dict(hemisphere, samples=count * in_reach), path(name),
                           options.threads, options.size)
        _, _, pixels = read_raw(path(name))
        rows.append(('hemisphere', count * in_reach, count * in_reach, wall, cpu, rms(pixels, hemisphere_reference),
                     rms(pixels, hemisphere_reference, mask)))

    print('Indirect-lit region: %d of %d pixels. %d objects in reach of a point.\n'
          % (sum(mask), len(mask), in_reach))
    print('| Method | Rays (per object / per point) | Rays a point | Wall (s) | CPU (s) | RMS, whole image | RMS, indirect-lit |')
    print('|---|---|---|---|---|---|---|')
    for method, count, per_point, wall, cpu, whole, region in rows:
        print('| %s | %d | %d | %.1f | %.1f | %.2f | %.2f |' % (method, count, per_point, wall, cpu, whole, region))

    if options.image:
        from PIL import Image, ImageDraw, ImageFont
        columns = 4
        scale = 2
        label = 34
        tile_w, tile_h = width * scale, height * scale
        rows_needed = (len(tiles) + columns - 1) // columns
        grid = Image.new('RGB', (columns * tile_w + (columns + 1) * 6, rows_needed * (tile_h + label) + (rows_needed + 1) * 6),
                         (255, 255, 255))
        draw = ImageDraw.Draw(grid)
        try:
            font = ImageFont.truetype('DejaVuSans-Bold.ttf', 15)
            small = ImageFont.truetype('DejaVuSans.ttf', 13)
        except OSError:
            font = small = ImageFont.load_default()
        for index, (name, count, whole, region) in enumerate(tiles):
            w, h, pixels = read_raw(path(name))
            image = Image.frombytes('RGB', (w, h), bytes(pixels)).resize((tile_w, tile_h), Image.NEAREST)
            x = 6 + (index % columns) * (tile_w + 6)
            y = 6 + (index // columns) * (tile_h + label + 6)
            draw.text((x, y), '%d ray%s an object' % (count, '' if count == 1 else 's'), fill=(0, 0, 0), font=font)
            draw.text((x, y + 17), 'RMS %.2f, indirect-lit %.2f' % (whole, region), fill=(60, 60, 60), font=small)
            grid.paste(image, (x, y + label))
        grid.save(options.image, optimize=True)
        print('\nWrote %s' % options.image, file=sys.stderr)


if __name__ == '__main__':
    main()
