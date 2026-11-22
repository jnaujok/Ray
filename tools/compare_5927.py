#!/usr/bin/env python3
"""Put NASA's AS11-40-5927 beside Ray's recreation and measure both in linear light.

    compare_5927.py <photo.jpg> <ray.png> [side_by_side.png]

photo.jpg is NASA's frame (any size, square; the ALSJ's 1950 x 1950 scan, for
example), ray.png a render of scenes/as11_40_5927_test.ray (any square size,
512 x 512 for the changelog). Both are brought to 512 x 512; the side-by-side
has the photograph on the left, Ray on the right, labelled, 8 pixels apart.
The table gives each region's mean in linear light (the sRGB curve undone),
its luminance, and blue over red. Regions are in the 512 x 512 frame; where
the recreation's part lies elsewhere in its own frame it has its own box.
Needs Pillow.
"""
import sys

from PIL import Image, ImageDraw

SIZE = 512
GAP = 8

    # name, the photograph's boxes, Ray's boxes (None: the same)
REGIONS = [
    ('sunlit ground', [(142, 416, 342, 486)], None),
    ('aft box face', [(277, 71, 337, 136)], None),
    ('copper Kapton', [(142, 206, 187, 261), (322, 206, 362, 256)], None),
    ('LM shadow', [(110, 298, 180, 315)], None),
    ('engine bell', [(250, 288, 272, 305)], None),
]


def linear(c):
    c /= 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def measure(image, boxes):
    px = []
    for b in boxes:
        px += list(image.crop(b).getdata())
    rgb = [sum(linear(p[i]) for p in px) / len(px) for i in range(3)]
    return rgb, 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]


def main(argv):
    if len(argv) not in (3, 4):
        raise SystemExit(__doc__)
    nasa = Image.open(argv[1]).convert('RGB').resize((SIZE, SIZE), Image.LANCZOS)
    ray = Image.open(argv[2]).convert('RGB').resize((SIZE, SIZE), Image.LANCZOS)
    print('| Region | NASA linear RGB | NASA Y | Ray linear RGB | Ray Y | Ray/NASA | b/r NASA, Ray |')
    print('|---|---|---|---|---|---|---|')
    for name, boxes, own in REGIONS:
        (n, ny), (r, ry) = measure(nasa, boxes), measure(ray, own or boxes)
        print('| %s | (%.3f, %.3f, %.3f) | %.3f | (%.3f, %.3f, %.3f) | %.3f | %.2f | %.2f, %.2f |' %
              (name, *n, ny, *r, ry, ry / max(ny, 1e-9), n[2] / max(n[0], 1e-9), r[2] / max(r[0], 1e-9)))
    if len(argv) == 4:
        both = Image.new('RGB', (2 * SIZE + GAP, SIZE), (255, 255, 255))
        both.paste(nasa, (0, 0))
        both.paste(ray, (SIZE + GAP, 0))
        d = ImageDraw.Draw(both)
        for x, label in ((0, 'NASA AS11-40-5927'), (SIZE + GAP, 'Ray')):
            d.rectangle((x + 6, 6, x + 12 + 7 * len(label), 24), fill=(0, 0, 0))
            d.text((x + 9, 9), label, fill=(255, 255, 255))
        both.save(argv[3])


if __name__ == '__main__':
    main(sys.argv)
