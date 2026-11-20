# Sample scenes

The `.ray` files here are the test scenes from the original development in 1996–1997, converted
to JSON, and the Utah teapot. The regression tests render every scene and compare it with the
reference renders in [`tests/reference/`](../tests/reference). The binary originals were removed
when the binary format was dropped; they are in the repository's history.

| Scene | Saved in 1997 as | Notes |
|-------|------------------|-------|
| `axis_test.ray` | version 2 | Six colored spheres along the axes. One is partly transparent; its index was 1.0 in 1997 and is 1.5 now |
| `inside_sphere.ray` | version 2 | Camera inside a sphere |
| `mat_test.ray` | version 2 | Material test |
| `reflect_me.ray`, `reflect_test2.ray`, `reflect_test3.ray`, `reflection_test.ray` | version 2 | Reflection tests |
| `scan_test.ray` | version 2 | Reflective and transparent spheres on a plane. The glass sphere's index was 1.0 in 1997, which looked like glass only because refraction was computed wrong; it is 1.5 now (as in `lens_test` and `chromatic_test`) |
| `ser_test.ray`, `ser_test2.ray` | version 2 | Serialization tests (`ser_test2` renders blank: nothing is in view) |
| `spec_test.ray` | version 2 | Specular highlights |
| `test.ray` | version 2 | |
| `triangle.ray`, `triangle2.ray` | version 2 | Triangle primitives. `triangle2` gives its corners tilted normals, which fake a curve (and are what light it: its lights lie in the triangle's plane) |
| `xxx.ray` | version 1 | Read with the legacy number format |
| `huh.ray` | transitional | Version-2 header with legacy numbers; renders blank |
| `lens_test.ray` | | `scan_test` through a real lens: the built-in double Gauss wide open (f/2), focused on the glass sphere, with 1 scene unit = 10 cm and a 5×5 stratified sampler |
| `chromatic_test.ray` | | `scan_test` through an uncorrected BK7 singlet (stop behind it), whose colors focus apart: chromatic aberration, plus the softness of one simple lens |
| `texture_test.ray` | | Textures: a checkered floor, `textures/uv_grid.ppm` on a wall and a sphere, and a solid checker sphere |
| `primitives_test.ray` | | Cylinders, cones and boxes: a capped cylinder, an open tube lying on its side (lit only where light gets in), a cone, a truncated cone and a turned cube with `textures/uv_grid.ppm`, and a turned glass box |
| `torture_test.ray` | | Everything at once, at 1024×768: the teapot (a mirror-tinted copper glaze) among a mirror sphere, a glass sphere, a textured sphere, a solid-checker column, an open gold tube, a cone, a textured truncated cone, a green glass cube, a textured box and a textured triangle, on a reflective checkered floor before a mirror wall in a black frame, through the double Gauss at f/2 with a 5×5 stratified sampler (25 rays a pixel, each traced in red, green and blue) |
| `teapot_textured.ray` | | The teapot with `textures/uv_grid.ppm`, spherically UV-mapped about its vertical axis |
| `teapot.ray` | | The Utah teapot (Martin Newell, 1975): a smooth, double-sided mesh of 6,320 triangles, from the triangulated model in [alecjacobson/common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models), converted with `tools/obj_to_ray.py --smooth --double-sided --weld` |

Two 1997 files, `yyy.ray` (saved by an in-between build that wrote an extra, undefined field) and
`stream_test1.ray` (debug-heap fill bytes, not a scene), could never be read and were not
converted.

## Scene files

A saved `.ray` file is an IASLib resource archive (`CResourceFile`, starting `IASRES`): resource
1/1 is `scene.json`, the scene below, and folder 2 holds every file the scene uses (texture images),
each named as the scene names it. Files a scene names are looked for in its archive first, then on
disk relative to the scene file. A `.ray` file can also be the plain JSON itself, as the sample
scenes here are; `raythrower scene.ray --save-json out.ray` writes one from an archive, with the
files it uses extracted beside it, and `--save` turns it back into an archive. A file named outside
the scene's directory (absolute, or climbing out with `..`) is stored under `files/`.

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
| `objects` | `[]` | Spheres, planes, cylinders, cones, boxes, triangles, and meshes, below |
| `lights` | `[]` | Lights, below |

**`settings`**

| Field | Default | |
|-------|---------|-|
| `imageWidth`, `imageHeight` | `640`, `480` | Image size in pixels |
| `imageFile` | `"ray.out"` | Where `raythrower` writes the image, unless given `-o` |
| `colorDepth` | `8` | Bits per channel (kept from 1997; images are always 8-bit) |
| `maximumDepth` | `16` | How many levels deep a ray may be traced: the ray from the camera is level 1, and each reflection or refraction one more, so 16 allows 15 bounces |
| `depthThreshold` | `0.01` | A ray whose influence on its pixel falls below this is not traced further. (Reflected and refracted rays keep their parent's influence, so in practice `maximumDepth` is what stops them.) |
| `reserved1997` | | Settings the 1997 code defined but never implemented (anti-aliasing, caustics, focal blur, haze, and so on), kept so nothing is lost. Only the ones that differ from their defaults are written. The tracer ignores them. |

**`camera`**

| Field | Default | |
|-------|---------|-|
| `position` | required | Where the camera is |
| `lookAt` | required | The point at the center of the image |
| `up` | `[0, 1, 0]` | Which way is up; the image's top edge faces this way |
| `verticalAngle` | `30` | Vertical field of view in degrees. The horizontal one follows from the image's shape, so pixels are square. |
| `horizontalAngle`, `aspectRatio` | `40`, `1.333…` | Kept from 1997; not used for rendering |
| `sampler` | `"none"` | How each pixel is sampled: `"none"` (one ray through its center), `"corners"` (five rays, at its corners and center, about two per pixel), `"box"` (four rays, halfway from the center to each corner), or `"random"` (a center ray worth half the pixel and N random rays sharing the other half), or `"stratified"` (an n×n grid of equally weighted rays, each at a random point in its cell) |
| `lens` | none | A real lens, below; without one the camera is a pinhole |
| `colorModel` | `"clip"` | How pixel colors become bytes: `"clip"` (clip at 1, as in 1997), `"gamma"`, `"srgb"`, `"reinhard"`, or `"hdr"` |

A sampler or color model is a name, or an object with a `type` and its settings:
`{ "type": "random", "samples": 9, "seed": 0 }` (N from 1 to 1024, default 4; the same seed always
gives the same image), `{ "type": "stratified", "grid": 5 }` (n from 1 to 32, default 4), `{ "type": "gamma", "gamma": 2.2 }`, or
`{ "type": "reinhard", "exposure": 1, "white": 4 }`
(`exposure` scales the colors first; with a `white` luminance, that and anything brighter maps to
white), or `{ "type": "hdr", "exposure": 1 }` (high dynamic range: the colors scaled by
`exposure` go unclipped into `raythrower --hdr` files; 8-bit files get them clipped and
sRGB-encoded).

**`lens`.** With a lens the camera is a real camera: rays start on the sensor and are traced back
through each glass surface out into the scene, and rays that hit the barrel or the aperture stop go
dark. Depth of field, vignetting, and the lens's aberrations and distortion all come from that. The
field of view comes from the focal length and `sensorWidth` (`verticalAngle` is not used), the
sensor is placed to focus at `focusDistance`, and exposure is set so the center of the image is as
bright as through a pinhole. Use a sampler with many evenly weighted rays (`stratified`), since
each ray takes a different path through the lens. Rays are aimed only at the exit pupil: for each
ring of the sensor out from the center, the lens works out (the first time a ray needs it) an
ellipse on its rear element around everywhere light gets through, and each ray counts for that
ellipse's share of the element, so brightness is the same as aiming at all of it but far fewer rays
are blocked. With `raythrower`, the lens is described before
tracing.

| Field | Default | |
|-------|---------|-|
| `prescription` | | A built-in lens: `"double-gauss-50mm"` (50 mm f/2, US patent 2,673,491, as tabulated by Kolb et al., 1995) |
| `surfaces` | | Or the lens's own surfaces, front (scene side) to back; give one or the other |
| `fNumber` | wide open | The aperture; can't be wider than the lens opens |
| `focusDistance` | the look-at point | Where it focuses, in scene units from the front of the lens |
| `sensorWidth` | `36` | In mm (36 is full frame); the height follows from the image's shape |
| `millimetersPerUnit` | `1000` | How big a scene unit is (1000: scene units are meters) |
| `seed` | `0` | Picks the points rays cross the lens at |
| `chromatic` | `true` | Trace red, green and blue apart where the glasses have Abbe numbers; `false` traces every channel at the d line, a third of the work |

Each surface: `radius` of curvature in mm (positive when its center of curvature is toward the
sensor; `0` for flat), `thickness` to the next surface in mm (the last one's is ignored: focusing
places the sensor), `ior` of what follows it (default `1`, air), `aperture`, its semi-aperture
(radius) in mm, `stop: true` for the aperture stop, which must be flat, and `abbe`, the Abbe number
of the glass that follows (leave it out for no dispersion).

**Chromatic aberration.** Glass bends short wavelengths more than long ones. Each glass's index is
fitted to a Cauchy curve, n = A + B/λ², through its `ior` (at the d line, 587.6 nm) and its Abbe
number, and each sample is traced three times: red at the C line (656.3 nm), green at the d line
and blue at the F line (486.1 nm), each keeping only its own channel. Where the paths part, colors
fringe; green uses `ior` as given, so focus and exposure are worked out for it. The built-in double
Gauss is corrected for color, so its fringes are faint; a single lens shows them plainly
(`chromatic_test.ray`). A prescription that
doesn't converge light, or can't focus at the distance asked, is reported when the scene loads.

**Objects.** Each has a `type`, a `color` (default `[0.5, 0.5, 0.5]`; the 1997 editor's base
color, not used in shading), and `materials` (default: one diffuse material).

| `type` | Fields |
|--------|--------|
| `sphere` | `center` (required), `radius` (required) |
| `plane` | `position` (required): a point on the plane; `normal` (required) |
| `cylinder` | `base` and `top` (required): the centers of its ends; `radius` (required, more than 0); `capped` (default `true`) |
| `cone` | `base` and `top` (required): the centers of its ends; `baseRadius` (required); `topRadius` (default `0`, a point; more makes a truncated cone); `capped` (default `true`) |
| `box` | `center` (required); `size` (required): one number for a cube, or `[x, y, z]`, each more than 0; `rotation` (default `[0, 0, 0]`): degrees about x, then y, then z |
| `triangle` | `corners` (required): three points; `doubleSided` (default `false`); `smooth` (default `false`, or `true` with `normals`); `normals`: one for each corner; `uvs`: a `[u, v]` for each corner |
| `mesh` | `vertices` (required): points; `faces` (required): three vertex indices each, from 0; `doubleSided` (default `false`); `smooth` (default `false`, or `true` with `normals`); `normals`: one for each vertex; `uvs`: one for each vertex, or `uvMap` (below) |

A cylinder or cone points from `base` to `top`, any way. Capped, its ends are closed with flat discs
and it is a solid; with `"capped": false` it is an open shell, seen into from its ends, and it
shadows and reflects itself inside. A cone's radii can't be negative or both 0, and `base` and
`top` can't be the same point. A box is centered on `center`, `size` long along its own axes, which
`rotation` turns from the scene's x, y and z.

A triangle's normal is worked out from its corners by the edge traversal (V2 − V1) × (V3 − V2), so
the order of the corners decides which side is the front. It is the rule OBJ and most other model
formats are made for. A single-sided triangle can only be hit
from the front; a double-sided one can be hit from either side and is shaded from the side it is
seen from. A flat triangle is shaded with that normal everywhere. In a smooth mesh each vertex's
normal is the sum of the normals of the faces around it, and each face blends its corners' normals
by barycentric weights, so neighboring faces meet without a seam. (A lone smooth triangle has no
neighbors, so it shades flat.) A mesh's faces whose corners are in a line are left out.

`normals` overrides the corner (or vertex) normals that smooth shading blends, for a surface that
should curve differently than its faces suggest. They are made unit vectors and can't be
`[0, 0, 0]`. They don't change which side is the front or where rays hit, only shading, and they
make the shape smooth (`"smooth": false` with them is an error).

**Materials.** An object's look is the sum of its materials, each scaled by its `influence`
(default `1`). Each has a `type` and a `color`.

| `type` | Other fields |
|--------|--------------|
| `ambient` | none |
| `diffuse` | `diffuseCast` (default `false`) |
| `specular` | A Cook–Torrance highlight. `power` (default `1`) is the surface's Beckmann roughness: smaller is a smaller, sharper highlight (0.1–0.3 is glossy; 1 is very rough). `indexOfRefraction` (default `1.5`) sets the Fresnel reflection, stronger at grazing angles; `materialHighlight` (default `false`) |
| `reflective` | A mirror reflection, tinted by `color` with `filterReflection` (default `false`); `diffuseReflectivity` (default `0`) |
| `transparent` | Light passing through, bent by Snell's law at `indexOfRefraction` (default `1`, which doesn't bend; glass is about 1.5), and reflected inside past the critical angle; tinted by `color` |

**Textures.** Any material can have a `texture` that gives its color at each point instead of its
`color`. A texture is an object with a `type`:

| `type` | Fields |
|--------|--------|
| `checker` | `colors` (two colors, default black and white), `size` (default `1`), `solid` (default `false`: squares in surface coordinates; `true`: cubes in the scene, which needs no surface coordinates) |
| `image` | `file` (required: a PPM, P6 or P3, or a Ray raw image, relative to the scene file), `scale` (`[u, v]` repeats, default `[1, 1]`), `offset` (`[u, v]`, default `[0, 0]`), `repeat` (default `true`; `false` stretches the edge pixels), `srgb` (default `false`: a byte is the color /255, matching the default color model; `true` decodes the sRGB curve) |

Surface coordinates: on a sphere, u is longitude (once around, left to right as seen from −z) and v
latitude (0 at the bottom, 1 at the top). On the side of a cylinder or cone, u is the angle about
its axis (running as on a sphere when it stands on +y) and v goes from 0 at the base to 1 at the
top; each cap holds the unit square, as seen from outside it. Each face of a box holds the unit
square, upright as seen from outside: the sides with v up, the top seen from above with +z up and
the bottom seen from below with −z up (all before `rotation`). On a plane, u and v are distances in scene units along
two axes across it (on a floor, +x and +z; on a wall facing −z, +x and up). Triangles take `uvs`
(a `[u, v]` for each corner) and meshes `uvs` (one for each vertex); without them only solid
textures apply, unless a mesh has a `uvMap`, which works out each face's coordinates by projecting
it about an axis:

| `uvMap` field | Default | |
|---------------|---------|-|
| `type` | required | `planar` (along the axis, 0 to 1 across the bounding box), `cylindrical` (u the angle about the axis, v the height along it), `spherical` (u longitude, v latitude about the center), or `box` (each face planar along the axis it faces most) |
| `axis` | `"y"` | `"x"`, `"y"` or `"z"`. Along y, u follows x and v follows z; along z, x and y; along x, z and y. Angles run as on a sphere |
| `center` | the bounding box's center | For cylindrical and spherical maps |
| `scale` | `[1, 1]` | Repeats in u and v |

Faces across a cylindrical or spherical map's seam are unwrapped rather than smeared across the
whole texture, and a corner on the axis (a pole) takes its face's other corners' angle. An image's (0, 0) is its bottom left corner and (1, 1) its top right; pixels are
blended bilinearly. When a scene is copied to a task its images travel inside it (as base64
`pixels`), so networked engines need no files; a saved scene carries its images in its archive.

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
  camera's sampler and color model, triangle sidedness and smoothing, meshes, and cylinders, cones
  and boxes; triangles no
  longer store normals. The binary versions are no longer read.

`renders/` holds output files from 1997. `scan_test.raw` there is a partial trace from that time
(a few scan lines over black), not a reference image.
