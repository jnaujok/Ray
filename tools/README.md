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

## `make_teapot_bpt.py`

Writes `scenes/models/teapot.bpt`, the Utah teapot's 32 Bezier patches, from the copy of Martin
Newell's data in three.js's `TeapotGeometry.js` (release r160): the coordinates as they are, in the
`.bpt` format the `bezier` object reads. It needs only Python 3, and the file once written needs
nothing:

```sh
curl -sLO https://raw.githubusercontent.com/mrdoob/three.js/r160/examples/jsm/geometries/TeapotGeometry.js
tools/make_teapot_bpt.py TeapotGeometry.js scenes/models/teapot.bpt
```

## `bounce_sweep.py`

Measures indirect light's noise and render time against its number of rays. It renders a scene
(`scenes/bounce_test.ray` by default) with the `"objects"` method at 1, 4, 9, 16, 25, 36, 49 and 64
rays an object, and with the `"hemisphere"` method at the same rays a point for a few of them, timing
each render and measuring its noise as the root mean square difference (0–255) from a reference with
256 rays an object, over the whole image and over the pixels only bounced light lights. It prints a
Markdown table and, with `--image`, writes a labelled grid of the renders (that needs Pillow).
`--scene`, `--size`, `--counts`, `--hemisphere` and `--reference` measure other scenes and counts.
It is a measurement, not a test; the 2026-alpha-0.14.1 changelog has its results.

```sh
tools/bounce_sweep.py build/raythrower --out sweep --image sweep/grid.png
tools/bounce_sweep.py build/raythrower --scene scenes/torture_test.ray --size 160x120 \
    --counts 1,4,9 --hemisphere 1,4,9 --reference 49 --out sweep-torture
```

## `bench.py`

Times `raythrower` on the scenes 2026-alpha-0.21 was tuned on (the torture test at 240×180 with the
full-size adaptive settings, Benchy, the depth-of-field chessboard, the fractals, the AS11-40-5903
recreation and a render by three networked engines) and prints each one's wall time, CPU time, peak
memory and an MD5 of the image, so a change can be checked both for speed and for giving exactly
the same bytes. It needs GNU time (`/usr/bin/time`). `--runs` keeps the fastest of several runs,
`--only` picks cases, `--tasks` sets the threads (default 16) and `--json` saves the numbers.

```sh
python3 tools/bench.py --runs 2 --json before.json
```

## `adaptive_sweep.py`

Measures the adaptive sampler's noise against its cost, to choose its settings:

```sh
python3 tools/adaptive_sweep.py build/raythrower --scene scenes/torture_test.ray --size 240x180
```

Renders each scene with a reference sampler (256 rays a pixel) and with each `--sampler` (the
original rule and the confidence-aware, per-channel one at several settings by default), and prints
a table of rays a pixel, wall and CPU time, and the root mean square error against the reference,
over the whole image and its worst tenth of pixels. Each adaptive render also leaves a heat map of
its rays a pixel (`raythrower --ray-count`). It found the full-size torture test's settings in
2026-alpha-0.20.1.

## `make_showcase_scenes.py`

Writes `scenes/chess_dof_test.ray` (the Najdorf Sicilian on a full board in depth of field, the
position played out by python-chess: `pip install chess`), `scenes/as11_40_5903_test.ray` (the
AS11-40-5903 recreation) and the indirect-light study scenes in `scenes/studies/`.

## `make_lens_cutaways.py`

Writes `scenes/cutaways/<prescription>.ray`, every built-in lens built as glass (`lensElements`) and
cut away with its ray fans, and `scenes/lens_cutaway_test.ray`, the double Gauss small for the test
suite. It reads the prescription tables from `core/src/Lens.cpp` and frames each scene from the
lens's length and its paraxial back focal distance. Only the Python standard library is needed.

## `make_lm5_eagle.py`

Writes `scenes/models/apollo/lm5_eagle.glb`, the LM-5 Eagle, from NASA's
`apollo_lunar_module.glb` (`pip install DracoPy numpy Pillow`): decodes it, turns round the faces NASA
wound against their own normals, and sorts every face into one of 16 named parts (the glTF
materials of the output, which a mesh's `materialMap` dresses) by its NASA material, its node, its
height and which way it faces. The ascent stage's skin is split by facing: low faces facing forward
are black Pyromark paint, low aft faces beside the RCS quads crinkled silver foil, sideways and aft
faces the champagne 5056 anodized panels, the rest the gray 2024 skin, and forward, high faces of
the window frames quartz; the secondary struts from the descent stage out to the legs are gold strut
wrap (`kapton_gold`). It works normals out with a
45° crease, scales the model by 1.4 to the real LM's leg span and its height by 0.948 more to the real proportions, shortens the primary struts' lower
sections so everything above the footpads comes down 15 cm, stands it on y = 0, splits the
anodized skin into flat panels (each bellied out by tilting its edge normals 5° away from its
middle), packs them into a texture atlas (`lm5_eagle_panels.png`: each panel's color, varied a
little, with its seam and rivets; `lm5_eagle_rivets.png`: the rivets' heights) and writes an
indexed, uncompressed glb with their texture coordinates. The same files come out every time.

## `make_eagle_scenes.py`

Writes `scenes/models/apollo/lm5_eagle.ray`, the Eagle prefab (one mesh with a `materialMap`, and a
camera and sun of its own), `scenes/as11_40_5927_test.ray`, the AS11-40-5927 recreation that
brings it in with an `include`, and `scenes/terrain/apollo11_bake.ray`, the scene its surroundings
are baked from (see [`../scenes/terrain/README.md`](../scenes/terrain/README.md)). The recreation
stands on the LROC terrain (`terrain/apollo11_inner.glb`) under the sun as it stood at GET
110:53:38 (14.96° up at azimuth 90.2°), with the baked surroundings as its `environment`, a rock
half sunk in the ground, a bag and a pole, the LM turned and tilted onto the ground and 4 cm down
(its footpads in the regolith), and the camera where `solve_5927_view.py` puts it, exposed as the
Apollo crews did down-sun (f/11 at 1/250 s). Only the Python standard library is needed.

## `solve_5927_view.py`

Solves the AS11-40-5927 view from the photograph: the LM's heading, the camera's azimuth and
distance from the LM, and its pan and tilt, by fitting the four footpads' places in NASA's
1950 × 1950 scan (Nelder–Mead from a grid of starts), with the camera held 1.03 m above the ground
under it and the LM tilted onto the plane it stands on, both read from
`scenes/terrain/apollo11_inner_crop.tif`. Prints the LM's rotation as Ray's x, y, z angles, the
camera's position and look-at, and each pad's place in the photograph and in the solved view,
which `make_eagle_scenes.py` copies. Needs numpy.

## `geotiff_terrain.py`

Crops a digital terrain model around a point and tessellates it into a glTF mesh:
`geotiff_terrain.py <model> --center LAT LON --radius METERS [--inner METERS] [--step N]
[--curvature] [--fill-center METERS] --out mesh.glb [--crop crop.tif]`. It reads PDS3 `.IMG`
products with an attached label (such as the LROC NAC DTMs: 32-bit floats on an equirectangular
grid) and plain GeoTIFFs (uncompressed, strips or tiles, float32 or int16, equirectangular or
geographic, with a nodata value). The mesh is in local meters (x east, y up, z north) about the
center, two triangles to each grid cell inside the radius (and outside `--inner`, for a ring that
goes round another mesh), every `--step`th post, with smooth normals and a regolith material; it
stops where the model has no data. `--curvature` drops the ground by d²/2R for the Moon's
curvature; `--fill-center` replaces the middle (where a lander shows as a bump) by the plane fitted
to the ground from that radius to twice it, and puts y = 0 on that plane under the center.
`--crop` writes the heights as a float32 GeoTIFF in the same local meters (which it reads back),
and `--sample LAT LON` and `--sample-local EAST NORTH` print the ground's height at points. Needs
numpy.

## `hdr_rle.py`

Rewrites a Radiance HDR file (`raythrower --hdr` writes flat pixels) with run-length encoded
scanlines, pixel for pixel the same: `hdr_rle.py in.hdr [out.hdr]`. A baked lunar panorama, half
black sky, comes out at under half the size. Only the Python standard library is needed.

## `compare_5927.py`

Puts NASA's AS11-40-5927 beside a render of `as11_40_5927_test.ray`, both at 512 × 512, labelled,
and prints a table of regions measured in linear light in each (the sunlit ground, the aft box's
face, the copper Kapton, the ground in the LM's shadow, the engine bell): `compare_5927.py
photo.jpg render.png [side_by_side.png]`. Needs Pillow.

## `make_icon.py`

Draws `packaging/io.github.jnaujok.Ray.png`, the 256×256 PNG of Ray's icon, the same picture as
`packaging/io.github.jnaujok.Ray.svg` (a lens bending three rays to a focus). Needs Pillow.

## `gltf_decompress.py`

Writes a Draco-compressed `.glb` back out with plain accessors (`pip install DracoPy numpy`). Ray
decodes Draco itself since 2026-alpha-0.20.2, so this is only needed to edit a model's vertices with
a plain script: `pose_astronaut.py` works on its output.

## `make_blend_fixtures.py`

Writes the small Blender files in `scenes/models/` (`blend_legacy.blend` and `blend_modern.blend`,
one in each of Blender's mesh layouts, and their gzip and Zstandard copies) for `format_test` and
`blend_test.ray`: a header, blocks and a structure catalog the way Blender writes them, the same
every time. Zstandard needs `pip install zstandard`.

## `changelog_section.py`

Prints one release's section of `CHANGELOG.md` (`changelog_section.py <tag> [CHANGELOG.md]
[owner/repo]`), with links and images made absolute to the files at that tag, for the GitHub
Release's notes; `.github/workflows/release.yml` uses it.

## `make_image_fixtures.py`

Writes the JPEG and WebP test images in `scenes/textures/formats/` for `format_test`, each with
Pillow's decode beside it (`<name>_ref.png`, and `<name>_alpha.png` for images with alpha), which
Ray's decoders are compared against: baseline and progressive JPEGs at 4:4:4, 4:2:2 and 4:2:0,
grayscale, restart markers, Exif orientations 3, 6 and 8 and a CMYK file (which Ray refuses);
lossless WebPs (every transform, palettes, alpha) and lossy ones (with and without alpha). The lossy
WebPs that need encoder settings Pillow doesn't offer (the simple loop filter, sharpness, several
token partitions, one segment) are written by a small C program it builds against the system's
libwebp (`libwebp-dev`), when there is one. It also writes `bricks.jpg` and `bricks.webp` from
`../bricks.png` for `images_test.ray`. Needs Pillow; the same files come out every time with the
same library versions.

## `pose_astronaut.py`

Lowers the arms of NASA's astronaut model (decompressed) from its T-pose, about the shoulders, for
the AS11-40-5903 recreation.

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

## `make_shape_scenes.py`

Writes `scenes/lathe_test.ray` and `scenes/extrusion_test.ray` from `shapes_test.ray`'s settings
(camera, lights, floor): chess pieces, a goblet, a vase and a cut orange turned on a lathe, and a
tapered star, a twisted column, a beveled frame, a spring, a closed loop and a rail. The star's
points, the spring's helix and the loop are worked out here. It needs only Python 3; re-save each
scene with `raythrower` afterwards so it is written the way Ray writes scenes:

```sh
tools/make_shape_scenes.py
build/raythrower scenes/lathe_test.ray --save-json scenes/lathe_test.ray
build/raythrower scenes/extrusion_test.ray --save-json scenes/extrusion_test.ray
```

## `make_math_scenes.py`

Writes `scenes/implicit_test.ray`, `scenes/blob_test.ray` and `scenes/fractal_test.ray` from
`shapes_test.ray`'s settings: implicit surfaces (a sphere's formula, a gyroid cut to a ball by a csg,
Taubin's heart), metaballs (a melted cluster and a dented ball) and fractals (a mandelbulb, a Menger
sponge and a Sierpinski tetrahedron). It needs only Python 3; re-save each scene with `raythrower`
afterwards:

```sh
tools/make_math_scenes.py
build/raythrower scenes/implicit_test.ray --save-json scenes/implicit_test.ray
build/raythrower scenes/blob_test.ray --save-json scenes/blob_test.ray
build/raythrower scenes/fractal_test.ray --save-json scenes/fractal_test.ray
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
