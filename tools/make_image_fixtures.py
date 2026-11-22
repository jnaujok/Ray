#!/usr/bin/env python3
"""Writes the JPEG and WebP test images in scenes/textures/formats/ for format_test.

    make_image_fixtures.py [scenes/textures/formats]

It also writes bricks.jpg and bricks.webp, ../bricks.png as JPEG and lossy
WebP, which images_test.ray shows beside the PNG.

Each image comes with a reference decode by Pillow (libjpeg-turbo and libwebp),
saved as a PNG beside it (<name>_ref.png, RGB, and for images with alpha
<name>_alpha.png, grayscale). format_test decodes the image with Ray's own
decoder and compares it with the reference: lossless WebP must match exactly;
lossy WebP and JPEG within the tolerance format_test states.

Pillow writes the JPEGs (baseline and progressive, 4:4:4, 4:2:2 and 4:2:0,
grayscale, restart markers, an Exif orientation) and the plain WebPs.
The lossy WebPs that need encoder settings Pillow doesn't offer (the simple
loop filter, sharpness, several token partitions, one segment) are written by
a small C program compiled against the system's libwebp (libwebp-dev), if a
C compiler and libwebp are present; otherwise those files are left as they are.

The pictures are drawn from fixed numbers, so the same files come out every
time with the same library versions.
"""
import io
import os
import random
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageOps

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'scenes', 'textures', 'formats')


def picture(width, height, seed=1):
    """A test picture: gradients, hard edges, circles and fine noise."""
    rnd = random.Random(seed)
    im = Image.new('RGB', (width, height))
    px = im.load()
    for y in range(height):
        for x in range(width):
            px[x, y] = (int(255 * x / max(1, width - 1)), int(255 * y / max(1, height - 1)), (x * 7 + y * 3) % 256)
    draw = ImageDraw.Draw(im)
    for _ in range(6):
        x0, y0 = rnd.randrange(width), rnd.randrange(height)
        r = rnd.randrange(3, max(4, min(width, height) // 3))
        draw.ellipse((x0 - r, y0 - r, x0 + r, y0 + r), fill=(rnd.randrange(256), rnd.randrange(256), rnd.randrange(256)))
    draw.rectangle((width // 3, 2, width // 3 + 3, height - 3), fill=(255, 255, 255))
    draw.line((0, height - 1, width - 1, 0), fill=(0, 0, 0), width=2)
    for _ in range(width * height // 12):
        x, y = rnd.randrange(width), rnd.randrange(height)
        r, g, b = px[x, y]
        d = rnd.randrange(-40, 41)
        px[x, y] = (max(0, min(255, r + d)), max(0, min(255, g - d)), max(0, min(255, b + d)))
    return im


def alpha_for(im):
    a = Image.new('L', im.size)
    px = a.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            px[x, y] = 0 if (x < 4 and y < 4) else min(255, (x * 255) // max(1, w - 1) + (y % 5) * 9)
    return a


def save_ref(name, data):
    path = os.path.join(OUT, name)
    with open(path, 'wb') as f:
        f.write(data)
    im = Image.open(io.BytesIO(data))
    im = ImageOps.exif_transpose(im)
    has_alpha = im.mode in ('RGBA', 'LA') or (im.mode == 'P' and 'transparency' in im.info)
    if has_alpha:
        rgba = im.convert('RGBA')
        rgba.getchannel('A').save(os.path.join(OUT, name.rsplit('.', 1)[0] + '_alpha.png'), optimize=True)
    im.convert('RGB').save(os.path.join(OUT, name.rsplit('.', 1)[0] + '_ref.png'), optimize=True)


def jpeg(name, im, **options):
    buf = io.BytesIO()
    im.save(buf, 'JPEG', **options)
    save_ref(name, buf.getvalue())
    return buf.getvalue()


def webp(name, im, **options):
    buf = io.BytesIO()
    im.save(buf, 'WEBP', **options)
    save_ref(name, buf.getvalue())


C_ENCODER = r'''
#include <stdio.h>
#include <stdlib.h>
#include <webp/encode.h>
/* encoder <in.rgb> <width> <height> <out.webp> <filter_type> <sharpness> <partitions> <segments> <quality> <strength> */
static int Write(const uint8_t *data, size_t size, const WebPPicture *pic) {
    return fwrite(data, size, 1, (FILE *)pic->custom_ptr) == 1 || size == 0;
}
int main(int argc, char **argv) {
    if (argc < 11) return 2;
    int w = atoi(argv[2]), h = atoi(argv[3]);
    FILE *in = fopen(argv[1], "rb");
    uint8_t *rgb = malloc((size_t)w * h * 3);
    if (!in || fread(rgb, (size_t)w * h * 3, 1, in) != 1) return 3;
    fclose(in);
    WebPConfig config;
    WebPPicture pic;
    if (!WebPConfigInit(&config) || !WebPPictureInit(&pic)) return 4;
    config.quality = (float)atof(argv[9]);
    config.method = 4;
    config.filter_type = atoi(argv[5]);
    config.filter_sharpness = atoi(argv[6]);
    config.partitions = atoi(argv[7]);
    config.segments = atoi(argv[8]);
    config.filter_strength = atoi(argv[10]);
    config.autofilter = 0;
    pic.width = w; pic.height = h;
    if (!WebPPictureImportRGB(&pic, rgb, w * 3)) return 5;
    FILE *out = fopen(argv[4], "wb");
    pic.writer = Write; pic.custom_ptr = out;
    if (!WebPEncode(&config, &pic)) return 6;
    fclose(out);
    WebPPictureFree(&pic);
    return 0;
}
'''


def libwebp_variants(im):
    cc = shutil.which('cc') or shutil.which('gcc')
    if not cc or not os.path.exists('/usr/include/webp/encode.h'):
        print('no C compiler or libwebp-dev: the libwebp-only fixtures are left as they are')
        return
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, 'enc.c')
        exe = os.path.join(tmp, 'enc')
        with open(src, 'w') as f:
            f.write(C_ENCODER)
        if subprocess.call([cc, '-O2', src, '-o', exe, '-lwebp']) != 0:
            print('could not build the libwebp encoder: the libwebp-only fixtures are left as they are')
            return
        raw = os.path.join(tmp, 'in.rgb')
        with open(raw, 'wb') as f:
            f.write(im.tobytes())
        w, h = im.size
        variants = [
            # name, filter_type (0 simple, 1 normal), sharpness, log2 partitions, segments, quality, strength
            ('lossy_simple.webp', 0, 0, 0, 4, 70, 60),
            ('lossy_sharp.webp', 1, 5, 0, 4, 60, 70),
            ('lossy_parts.webp', 1, 0, 2, 1, 85, 30),
            ('lossy_low.webp', 1, 0, 3, 4, 10, 100),
        ]
        for name, ftype, sharp, parts, segs, q, strength in variants:
            out = os.path.join(tmp, name)
            if subprocess.call([exe, raw, str(w), str(h), out, str(ftype), str(sharp), str(parts), str(segs), str(q), str(strength)]) != 0:
                print('libwebp failed on', name)
                continue
            with open(out, 'rb') as f:
                save_ref(name, f.read())


def main():
    os.makedirs(OUT, exist_ok=True)
    photo = picture(61, 45)
    big = picture(130, 97, seed=2)

    # JPEG
    jpeg('base444.jpg', photo, quality=90, subsampling=0)
    jpeg('base422.jpg', photo, quality=85, subsampling=1)
    jpeg('base420.jpg', big, quality=75, subsampling=2)
    jpeg('prog420.jpg', big, quality=80, subsampling=2, progressive=True)
    jpeg('prog444.jpg', photo, quality=95, subsampling=0, progressive=True)
    jpeg('gray.jpg', photo.convert('L'), quality=85, progressive=True)
    jpeg('restart.jpg', big, quality=70, subsampling=2, restart_marker_blocks=3)
    exif = Image.Exif()
    exif[0x0112] = 6
    jpeg('exif6.jpg', photo, quality=90, exif=exif.tobytes())
    exif[0x0112] = 3
    jpeg('exif3.jpg', photo, quality=90, exif=exif.tobytes())
    exif[0x0112] = 8
    jpeg('exif8.jpg', photo, quality=90, exif=exif.tobytes())
    # Refused: CMYK
    buf = io.BytesIO()
    photo.convert('CMYK').save(buf, 'JPEG', quality=90)
    with open(os.path.join(OUT, 'cmyk.jpg'), 'wb') as f:
        f.write(buf.getvalue())

    # WebP
    webp('lossless.webp', big, lossless=True, method=6)
    webp('lossless_fast.webp', photo, lossless=True, method=0, quality=0)
    rgba = big.copy()
    rgba.putalpha(alpha_for(big))
    webp('lossless_alpha.webp', rgba, lossless=True, exact=True)
    for colors, name in ((2, 'palette2.webp'), (4, 'palette4.webp'), (12, 'palette12.webp'), (100, 'palette100.webp')):
        webp(name, photo.quantize(colors, dither=Image.Dither.NONE).convert('RGB'), lossless=True)
    webp('lossy.webp', big, quality=80)
    webp('lossy_small.webp', photo, quality=50, method=6)
    webp('lossy_alpha.webp', rgba, quality=75, alpha_quality=100)
    webp('lossy_alpha_lossy.webp', rgba, quality=60, alpha_quality=40)
    libwebp_variants(big)

    # The bricks as JPEG and lossy WebP, beside bricks.png in images_test.ray.
    bricks = Image.open(os.path.join(OUT, '..', 'bricks.png')).convert('RGB')
    bricks.save(os.path.join(OUT, 'bricks.jpg'), quality=85, subsampling=2)
    bricks.save(os.path.join(OUT, 'bricks.webp'), quality=80, method=4)


if __name__ == '__main__':
    main()
