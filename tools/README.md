# Tools

## `check_image.py`

Checks `raythrower`'s PNG and Radiance HDR output against the raw image it wrote: the PNG must
decode (with Python's own zlib) to exactly the raw pixels, and the HDR file must have the same size
and, with `--clip`, colors that clipped at 1 match the raw bytes (the `clip` color model) within
RGBE's precision, with some brighter than 1. The `image_output` test runs it.

```sh
raythrower scenes/scan_test.ray -o scan.raw --png scan.png --hdr scan.hdr
tools/check_image.py scan.raw --png scan.png
tools/check_image.py scan.raw --hdr scan.hdr --clip
```

## `render_previews.py`

Writes a PNG preview of every reference render in `tests/reference/renders.tar.gz` to
`docs/images/tests/`, shrunk to at most 320 pixels wide, for the README's "Expected output"
section. It needs only Python 3. Run it from the repository root whenever the references change:

```sh
tools/render_previews.py
```

## `make_textures.py`

Makes the textures in `scenes/textures/`. The bricks, which `bump_test.ray` and the torture test use:
`bricks.png` (the color, 8-bit RGB, each row with a different PNG filter), `bricks_normal.png` (a
tangent-space normal map, OpenGL convention, Adam7-interlaced) and `bricks_height.png` (the height
field the normal map comes from, 16-bit gray). Between them they use most of what Ray's PNG reader
must handle. It also makes `sky.hdr`, which `environment_test.ray` and the torture test use as
their environment: a 512×256 equirectangular afternoon sky (blue overhead, pale at the horizon,
clouds on a flat layer, low hazy hills and sandy ground), a run-length encoded Radiance HDR file
whose sun, 38° up and turned 50° from +z toward −x, is 60 times brighter than white. It needs only
Python 3, and writes the same files every time:

```sh
tools/make_textures.py
```

## `make_models.py`

Makes the small model files in `scenes/models/` that `models_test.ray`, the tests and the torture
test use: `color_cube.stl` (a puzzle cube of colored stickers with black gaps, a binary STL colored
the VisCAM/SolidView way), `magics_gem.stl` (an octahedron colored the Materialise Magics way, four
faces with their own colors and four with the model's) and `ring.obj` (a torus of quads with
texture coordinates and normals, written with negative indices and with lines a reader ignores).
It needs only Python 3, and writes the same files every time:

```sh
tools/make_models.py
```

## `obj_to_ray.py`

Turns a Wavefront OBJ model into a Ray mesh object (JSON), ready to put in a scene's `objects`
list. It needs only Python 3.

```sh
tools/obj_to_ray.py model.obj [--flip] [--smooth] [--double-sided] [--weld] \
                              [--scale S] [--translate X Y Z] > mesh.json
```

| Option | Meaning |
|--------|---------|
| `--flip` | Reverse each face's corners. Ray takes a face's front to be the side (V2 − V1) × (V3 − V2) points to, the rule OBJ files are made for, so this is only for the occasional model wound the other way. |
| `--smooth` | Shade as a smooth surface (per-vertex normals) rather than flat facets |
| `--double-sided` | Let faces be hit from behind, for open models you can see into |
| `--weld` | Merge vertices at the same position. Many models repeat vertices along the seams between their parts, which leaves visible seams in smooth shading. |
| `--scale`, `--translate` | Move the model into place |

Only vertices and faces are read; polygons are split into triangles. `raythrower --save` tidies the
layout of a scene the mesh has been pasted into. `scenes/teapot.ray` was made with
`--smooth --double-sided --weld`.
