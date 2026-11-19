# Sample scenes

The `.ray` files here are the test scenes from the original development in 1996–1997, converted
to JSON, and the Utah teapot. The regression tests render every scene and compare it with the
reference renders in [`tests/reference/`](../tests/reference). The binary originals were removed
when the binary format was dropped; they are in the repository's history.

| Scene | Saved in 1997 as | Notes |
|-------|------------------|-------|
| `axis_test.ray` | version 2 | Six colored spheres along the axes |
| `inside_sphere.ray` | version 2 | Camera inside a sphere |
| `mat_test.ray` | version 2 | Material test |
| `reflect_me.ray`, `reflect_test2.ray`, `reflect_test3.ray`, `reflection_test.ray` | version 2 | Reflection tests |
| `scan_test.ray` | version 2 | Reflective and transparent spheres on a plane |
| `ser_test.ray`, `ser_test2.ray` | version 2 | Serialization tests (`ser_test2` renders blank: nothing is in view) |
| `spec_test.ray` | version 2 | Specular highlights |
| `test.ray` | version 2 | |
| `triangle.ray`, `triangle2.ray` | version 2 | Triangle primitives. `triangle2` gave its corners tilted normals to fake a curve; triangles now work out their own, so it renders flat |
| `xxx.ray` | version 1 | Read with the legacy number format |
| `huh.ray` | transitional | Version-2 header with legacy numbers; renders blank |
| `teapot.ray` | | The Utah teapot (Martin Newell, 1975): a smooth, double-sided mesh of 6,320 triangles, from the triangulated model in [alecjacobson/common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models), converted with `tools/obj_to_ray.py --smooth --double-sided --weld` |

Two 1997 files, `yyy.ray` (saved by an in-between build that wrote an extra, undefined field) and
`stream_test1.ray` (debug-heap fill bytes, not a scene), could never be read and were not
converted.

## The JSON format (version 3)

A scene is one JSON object. Fields marked *required* must be there; any other field that is left
out takes the default shown. A field the program doesn't know is an error, so a misspelling is
reported rather than ignored, and every error names where it is (`objects[2].materials[0]: unknown
field "colour"`). Vectors and colors are `[x, y, z]` and `[red, green, blue]` arrays; colors are
linear, with 1 as full brightness, and may go above 1. Saved files round numbers to six decimal
places.

| Field | | |
|-------|-|-|
| `format` | required | `"Ray scene"` |
| `version` | required | `3` |
| `settings` | | Render settings, below |
| `background` | `[1, 1, 1]` | The color of rays that hit nothing |
| `camera` | required | Below |
| `objects` | `[]` | Spheres, planes, and triangles, below |
| `lights` | `[]` | Lights, below |

**`settings`**

| Field | Default | |
|-------|---------|-|
| `imageWidth`, `imageHeight` | `640`, `480` | Image size in pixels |
| `imageFile` | `"ray.out"` | Where `raythrower` writes the image, unless given `-o` |
| `colorDepth` | `8` | Bits per channel (kept from 1997; images are always 8-bit) |
| `maximumDepth` | `15` | How many times a ray may be reflected or refracted |
| `depthThreshold` | `0.01` | A ray whose influence on its pixel falls below this is not traced further |
| `reserved1997` | | Settings the 1997 code defined but never implemented (anti-aliasing, caustics, focal blur, haze, and so on), kept so nothing is lost. Only the ones that differ from their defaults are written. The tracer ignores them. |

**`camera`**

| Field | Default | |
|-------|---------|-|
| `position` | required | Where the camera is |
| `lookAt` | required | The point at the center of the image |
| `up` | `[0, 1, 0]` | Which way is up; the image's top edge faces this way |
| `verticalAngle` | `30` | Vertical field of view in degrees. The horizontal one follows from the image's shape, so pixels are square. |
| `horizontalAngle`, `aspectRatio` | `40`, `1.333…` | Kept from 1997; not used for rendering |
| `sampler` | `"none"` | How each pixel is sampled: `"none"` (one ray through its center) or `"corners"` (five rays, at its corners and center, about two per pixel) |
| `colorModel` | `"clip"` | How pixel colors become bytes: `"clip"` (clip at 1, as in 1997), `"gamma"`, `"srgb"`, or `"reinhard"` |

A sampler or color model is a name, or an object with a `type` and its settings:
`{ "type": "gamma", "gamma": 2.2 }`, or `{ "type": "reinhard", "exposure": 1, "white": 4 }`
(`exposure` scales the colors first; with a `white` luminance, that and anything brighter maps to
white).

**Objects.** Each has a `type`, a `color` (default `[0.5, 0.5, 0.5]`; the 1997 editor's base
color, not used in shading), and `materials` (default: one diffuse material).

| `type` | Fields |
|--------|--------|
| `sphere` | `center` (required), `radius` (required) |
| `plane` | `position` (required): a point on the plane; `normal` (required) |
| `triangle` | `corners` (required): three points; `doubleSided` (default `false`); `smooth` (default `false`) |
| `mesh` | `vertices` (required): points; `faces` (required): three vertex indices each, from 0; `doubleSided` (default `false`); `smooth` (default `false`) |

A triangle's normal is worked out from its corners by the edge traversal (V2 − V1) × (V3 − V2), so
the order of the corners decides which side is the front. It is the rule OBJ and most other model
formats are made for. A single-sided triangle can only be hit
from the front; a double-sided one can be hit from either side and is shaded from the side it is
seen from. A flat triangle is shaded with that normal everywhere. In a smooth mesh each vertex's
normal is the sum of the normals of the faces around it, and each face blends its corners' normals
by barycentric weights, so neighboring faces meet without a seam. (A lone smooth triangle has no
neighbors, so it shades flat.) A mesh's faces whose corners are in a line are left out.

**Materials.** An object's look is the sum of its materials, each scaled by its `influence`
(default `1`). Each has a `type` and a `color`.

| `type` | Other fields |
|--------|--------------|
| `ambient` | none |
| `diffuse` | `diffuseCast` (default `false`) |
| `specular` | `power` (default `1`), `indexOfRefraction` (default `1`), `materialHighlight` (default `false`) |
| `reflective` | `diffuseReflectivity` (default `0`), `filterReflection` (default `false`) |
| `transparent` | `indexOfRefraction` (default `1`) |

**Lights.**

| `type` | Fields |
|--------|--------|
| `point` | `position` (required), `color` (required) |

## Format history

- **Version 1** (before April 1997) was binary. It stored every floating-point value as two 4-byte
  integers, a whole part and a fraction of 2³¹, and read objects until the end of the file.
- **Version 2** (1997–1999) was binary too, but stored floating-point values as `%.18e` text and
  wrote an object count. For a few weeks files had the version-2 header but still used the old
  numbers; `CScene::Load` tries both.
- **Version 3** (2026) is JSON, with the same file extension. It adds the background color, the
  camera's sampler and color model, triangle sidedness and smoothing, and meshes; triangles no
  longer store normals. The binary versions are no longer read.

`renders/` holds output files from 1997. `scan_test.raw` there is a partial trace from that time
(a few scan lines over black), not a reference image.
