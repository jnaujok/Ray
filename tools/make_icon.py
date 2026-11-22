#!/usr/bin/env python3
"""Draw packaging/io.github.jnaujok.Ray.png, the 256x256 PNG of Ray's icon (the same
picture as io.github.jnaujok.Ray.svg: a lens bending three rays to a focus).

    make_icon.py [output.png]

Needs Pillow.
"""
import sys

from PIL import Image, ImageDraw

SCALE = 4
SIZE = 256 * SCALE


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "packaging/io.github.jnaujok.Ray.png"
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle([8 * SCALE, 8 * SCALE, 248 * SCALE, 248 * SCALE], radius=44 * SCALE, fill=(20, 26, 38, 255))
    for points, color in (([(28, 82), (114, 82), (222, 128)], (255, 79, 63, 255)),
                          ([(28, 128), (222, 128)], (255, 210, 63, 255)),
                          ([(28, 174), (114, 174), (222, 128)], (79, 140, 255, 255))):
        draw.line([(x * SCALE, y * SCALE) for x, y in points], fill=color, width=10 * SCALE, joint="curve")
    # The lens: the quadratic curves of the SVG, front and back.
    outline = []
    for i in range(41):
        t = i / 40
        outline.append(((104 + 46 * 4 * t * (1 - t) / 2) * SCALE, (40 + 176 * t) * SCALE))
    for i in range(41):
        t = 1 - i / 40
        outline.append(((104 - 26 * 4 * t * (1 - t) / 2) * SCALE, (40 + 176 * t) * SCALE))
    lens = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    ImageDraw.Draw(lens).polygon(outline, fill=(142, 197, 255, 140), outline=(207, 230, 255, 255), width=5 * SCALE)
    image = Image.alpha_composite(image, lens)
    draw = ImageDraw.Draw(image)
    draw.ellipse([(222 - 9) * SCALE, (128 - 9) * SCALE, (222 + 9) * SCALE, (128 + 9) * SCALE], fill=(255, 255, 255, 255))
    image.resize((256, 256), Image.LANCZOS).save(out, optimize=True)


if __name__ == "__main__":
    main()
