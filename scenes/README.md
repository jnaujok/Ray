# Sample scenes

The `.ray` files here are the test scenes from the original development in 1996–1997, converted
to JSON. The binary originals are in [`1997/`](1997). The regression tests render every scene,
compare it with the reference renders in [`tests/reference/`](../tests/reference), and check that
each original renders exactly what its JSON version does and converts to exactly that file
(`raythrower 1997/<scene>.ray --convert <scene>.ray`).

| Scene | 1997 format | Notes |
|-------|-------------|-------|
| `axis_test.ray` | version 2 | Six colored spheres along the axes |
| `inside_sphere.ray` | version 2 | Camera inside a sphere |
| `mat_test.ray` | version 2 | Material test |
| `reflect_me.ray`, `reflect_test2.ray`, `reflect_test3.ray`, `reflection_test.ray` | version 2 | Reflection tests |
| `scan_test.ray` | version 2 | Reflective and transparent spheres on a plane |
| `ser_test.ray`, `ser_test2.ray` | version 2 | Serialization tests (`ser_test2` renders blank: nothing is in view) |
| `spec_test.ray` | version 2 | Specular highlights |
| `test.ray` | version 2 | |
| `triangle.ray`, `triangle2.ray` | version 2 | Triangle primitives |
| `xxx.ray` | version 1 | Read with the legacy number format |
| `huh.ray` | transitional | Version-2 header with legacy numbers; renders blank |
| `1997/yyy.ray` | **unreadable** | Saved by an in-between build that wrote an extra, undefined field |
| `1997/stream_test1.ray` | **unreadable** | An early stream test: debug-heap fill bytes, not a scene |

Many numbers in the converted scenes look like `0.6999999997206032` rather than `0.7`. That is
exactly what the 1997 files hold, and keeping it exact is what makes the renders identical.

## The JSON format (version 3)

A scene is one JSON object. Fields marked *required* must be there; any other field that is left
out takes the default shown. A field the program doesn't know is an error, so a misspelling is
reported rather than ignored, and every error names where it is (`objects[2].materials[0]: unknown
field "colour"`). Vectors and colors are `[x, y, z]` and `[red, green, blue]` arrays; colors are
linear, with 1 as full brightness, and may go above 1.

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
| `verticalAngle` | `30` | Vertical field of view in degrees. The horizontal one follows from the image's shape, so pixels are square. (`verticalAngleRadians` gives it in radians instead; the converter uses that when no number of degrees gives back the exact 1997 value.) |
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
| `triangle` | `corners` (required): three points; `surfaceNormal` (required); `normals` (required): one per corner |

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
- **Version 3** (2026) is JSON, with the same file extension. It adds the background color and the
  camera's sampler and color model. `CScene::Load` tells the formats apart by their first byte, and
  saving always writes version 3.

`renders/` holds output files from 1997. `scan_test.raw` there is a partial trace from that time
(a few scan lines over black), not a reference image.
