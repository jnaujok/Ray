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
| `torture_test.ray` | | Everything at once, at 1024×768: the teapot (polished marble) among a mirror sphere, a frosted glass sphere, a textured sphere, a solid-checker column, an open brushed and hammered gold tube, a hammered cone, a textured truncated cone, a green glass cube that absorbs by thickness, a brick box (a PNG texture with a PNG normal map), a textured triangle, a flint glass prism that splits what is seen through it into colors, and a die with drilled pips (both csgs), on a reflective checkered floor before a mirror wall in a wooden frame whose grain is bumped, lit by a sphere light, a point light, a rectangle light, a soft-edged spot light on the teapot and a low sun with an angle, under the HDR sky `textures/sky.hdr` (turned so its sun is over the key light), which shows in the mirrors and glass and lights the diffuse surfaces, through the double Gauss at f/2 with a 5×5 stratified sampler (25 rays a pixel, each traced in red, green and blue), each sample clamped at 1.5 |
| `solid_textures_test.ray` | | Solid textures and absorbing glass: a white marble sphere, a green marble cube, a wooden log and a wooden floor before a granite wall, three green glass slabs 0.15, 0.5 and 1.2 thick (the thicker, the darker), and an amber glass ball |
| `bump_test.ray` | | Bump and normal maps, and PNG textures: a brick wall (a PNG color texture and an interlaced PNG normal map) lit from the side, a hammered copper ball and an orange-peel ball (`noise` height maps), a box embossed with bricks by a 16-bit grayscale PNG height map, and a wood floor bumped by its own grain, with a 3×3 stratified sampler |
| `lights_test.ray` | | Spot and directional lights: a warm spot with a hard edge on a box, a white spot with a soft edge on a ball, a blue spot with a soft edge that is also a disc (`radius`), so the cone's shadows are soft, and a dim low sun with an angle of 4°, whose shadows blur the farther they fall, with a 4×4 stratified sampler |
| `environment_test.ray` | | An environment map: the HDR sky `textures/sky.hdr` behind the scene, in a mirror ball and through a glass one, lighting a white ball, a small red one and a checkered floor (`samples` 8, so the floor darkens under the balls), with a sun whose direction matches the sky's, with a 4×4 stratified sampler |
| `soft_test.ray` | | Rough materials and area lights: three glass panes before a striped poster, clear, lightly frosted (roughness 0.08) and heavily frosted (0.3), a mirror ball and a brushed gold one (0.12), a post and a red ball casting soft shadows from a rectangle light overhead and a sphere light to the right, with a 6×6 stratified sampler |
| `csg_test.ray` | | Constructive solid geometry: a glass lens blank (two balls' intersection), a bowl (a ball with a smaller one and a turned box taken out, blue outside, white inside and clay at the cut rim, by `childMaterials`), a die (a rounded cube, itself the intersection of a cube and a ball, with six pips drilled in and painted black) and a hammered gold capsule (a cylinder and two balls, with a bump map), with a 3×3 stratified sampler |
| `prism_test.ray` | | Dispersion: a flint glass prism (an intersection of three turned boxes) and a flint glass ball (index 1.62, Abbe number 20) before white bars on a black wall, which they split into red, green and blue, with a 2×2 stratified sampler |
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
| `background` | `[1, 1, 1]` | The color of rays that hit nothing, unless there is an `environment` |
| `environment` | none | What rays that leave the scene see instead: a sky gradient or an HDR image of the surroundings, which can also light the scene. Below |
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
| `depthThreshold` | `0.01` | A ray whose influence on its pixel falls below this is not traced further. A reflected or refracted ray carries its parent's influence times its material's share, so rays off a weakly reflective surface stop after a few bounces; between strong mirrors `maximumDepth` stops them |
| `reserved1997` | | Settings the 1997 code defined but never implemented (anti-aliasing, caustics, focal blur, haze, and so on), kept so nothing is lost. Only the ones that differ from their defaults are written. The tracer ignores them. |

**`camera`**

| Field | Default | |
|-------|---------|-|
| `position` | required | Where the camera is |
| `lookAt` | required | The point at the center of the image |
| `up` | `[0, 1, 0]` | Which way is up; the image's top edge faces this way |
| `verticalAngle` | `30` | Vertical field of view in degrees. The horizontal one follows from the image's shape, so pixels are square. |
| `horizontalAngle`, `aspectRatio` | `40`, `1.333…` | Kept from 1997; not used for rendering |
| `sampler` | `"none"` | How each pixel is sampled: `"none"` (one ray through its center), `"corners"` (five rays, at its corners and center, about two per pixel), `"box"` (four rays, halfway from the center to each corner), or `"random"` (a center ray worth half the pixel and N random rays sharing the other half), `"stratified"` (an n×n grid of equally weighted rays, each at a random point in its cell), or `"adaptive"` (more rays only where a pixel's rays disagree) |
| `lens` | none | A real lens, below; without one the camera is a pinhole |
| `colorModel` | `"clip"` | How pixel colors become bytes: `"clip"` (clip at 1, as in 1997), `"gamma"`, `"srgb"`, `"reinhard"`, or `"hdr"` |
| `clamp` | `0` (off) | The brightest any one sample's red, green or blue may be before the sampler averages it in. A rare ray that finds a light through rough glass, off a rough mirror or from an area light can be far brighter than its pixel's other rays and show as a lone bright speck (a "firefly"); a clamp of 1.5 to 2 removes them, at the cost of a little of the brightest highlights. Can't be negative |

A sampler or color model is a name, or an object with a `type` and its settings:
`{ "type": "random", "samples": 9, "seed": 0 }` (N from 1 to 1024, default 4; the same seed always
gives the same image), `{ "type": "stratified", "grid": 5 }` (n from 1 to 32, default 4),
`{ "type": "adaptive", "min": 9, "max": 64, "threshold": 0.01 }` (each pixel gets at least `min`
rays, then more until the standard error of its brightness, on the scale where 1 is white, is under
`threshold`, or it has `max`; rays follow a low-discrepancy sequence, reproducible like the others),
`{ "type": "gamma", "gamma": 2.2 }`, or
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
are blocked. A ray the lens does block is cast again at another point (up to 64 times) until one gets
through, and it counts for the share of rays that get through from that point of the sensor, which
the lens measures once: brightness and vignetting stay as they are, without grain from blocked
rays. With `raythrower`, the lens is described before
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
| `exitPupil` | | Written by Ray: the lens's exit pupil tables (base64) and a `key`, a fingerprint of the setup they are for. Tasks and later loads use them instead of working them out; if the lens, focus, aperture or sensor changes, the key no longer matches and they are worked out again, so it can be left alone or deleted |

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
color, not used in shading), `materials` (default: one diffuse material), and optionally a `bump`
(a bump or normal map; see **Bump and normal maps** below).

| `type` | Fields |
|--------|--------|
| `sphere` | `center` (required), `radius` (required) |
| `plane` | `position` (required): a point on the plane; `normal` (required) |
| `cylinder` | `base` and `top` (required): the centers of its ends; `radius` (required, more than 0); `capped` (default `true`) |
| `cone` | `base` and `top` (required): the centers of its ends; `baseRadius` (required); `topRadius` (default `0`, a point; more makes a truncated cone); `capped` (default `true`) |
| `box` | `center` (required); `size` (required): one number for a cube, or `[x, y, z]`, each more than 0; `rotation` (default `[0, 0, 0]`): degrees about x, then y, then z |
| `triangle` | `corners` (required): three points; `doubleSided` (default `false`); `smooth` (default `false`, or `true` with `normals`); `normals`: one for each corner; `uvs`: a `[u, v]` for each corner |
| `mesh` | `vertices` (required): points; `faces` (required): three vertex indices each, from 0; `doubleSided` (default `false`); `smooth` (default `false`, or `true` with `normals`); `normals`: one for each vertex; `uvs`: one for each vertex, or `uvMap` (below) |
| `csg` | `operation` (required): `"union"`, `"intersection"` or `"difference"`; `objects` (required): two or more closed shapes; `childMaterials` (default `false`) |

A cylinder or cone points from `base` to `top`, any way. Capped, its ends are closed with flat discs
and it is a solid; with `"capped": false` it is an open shell, seen into from its ends, and it
shadows and reflects itself inside. A cone's radii can't be negative or both 0, and `base` and
`top` can't be the same point. A box is centered on `center`, `size` long along its own axes, which
`rotation` turns from the scene's x, y and z.

**Constructive solid geometry.** A `csg` is a solid made from others: a `"union"` is inside any of
them, an `"intersection"` inside all of them, and a `"difference"` inside the first and none of the
rest. Its `objects` are written as in the scene's own list, and can be any closed shapes: spheres,
boxes, capped cylinders and cones, meshes (which must be closed; a mesh with holes gives wrong
results) and other csgs. Planes, triangles and open cylinders and cones are refused. A csg has
its own `materials` and `bump`, and shades as one object, so glass made of parts is entered and
left as one piece; its parts' materials and bump maps are ignored. With `"childMaterials": true`
each surface is shaded with the materials of the shape it comes from instead (a nested csg's
surfaces with those of its own shapes), so a die's drilled pips can be black on a white body. A
csg shadows and reflects itself: a bowl's inside is shaded and reflects its own rim. Its surface
coordinates are those of the shape each surface comes from. Each ray asks every shape for all its
crossings, so a csg costs a few times what its shapes do. A shape's normal at the solid's surface
is turned inside out where it is subtracted.

```json
{ "type": "csg", "operation": "difference", "childMaterials": true, "objects": [
    { "type": "box", "center": [0, 0.5, 0], "size": 1, "materials": [ { "type": "diffuse", "color": [1, 1, 1] } ] },
    { "type": "sphere", "center": [0, 1.03, 0], "radius": 0.09, "materials": [ { "type": "diffuse", "color": [0, 0, 0] } ] } ] }
```

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
| `reflective` | A mirror reflection, tinted by `color` with `filterReflection` (default `false`); `diffuseReflectivity` (default `0`). `roughness` (default `0`, a perfect mirror; up to `1`) blurs the reflection, as of brushed or satin metal: each ray reflects about a microfacet normal drawn from a GGX distribution of that roughness (0.05 is satin, 0.3 very rough) |
| `transparent` | Light passing through, bent by Snell's law at `indexOfRefraction` (default `1`, which doesn't bend; glass is about 1.5), and reflected inside past the critical angle; tinted by `color` at each surface it crosses. `attenuationColor` (default `[1, 1, 1]`, each 0 to 1) and `attenuationDistance` (default `0`, off) absorb light by how far it goes inside: after `attenuationDistance` it is `attenuationColor`, after twice that the color squared (Beer–Lambert). `roughness` (default `0`, clear; up to `1`) frosts the glass: each ray bends through a microfacet normal drawn from a GGX distribution of that roughness, so what is behind is blurred (0.05 lightly frosted, 0.3 heavily). `abbe` (default `0`, no dispersion; otherwise at least 5) is the glass's Abbe number: a ray splits into red, green and blue at the glass, each bent by its own index from a Cauchy curve through `indexOfRefraction` (green, the d line) and the Abbe number, as a real lens's glass is (crown glass is about 60, flint 30 to 40, very dense flint 20) |

Absorption is measured from where a ray goes into an object through its transparent material to
where it next meets that object, so the object should be closed (a sphere, a capped cylinder or
cone, a box, or a closed mesh). A ray turned back inside past the critical angle keeps absorbing.
Glass inside other glass is not tracked: a ray coming out of the inner piece is taken to be
outside both. Shadows are not tinted; glass shadows like any other object. For glass whose color
deepens with thickness, set `color` to white and give the color with `attenuationColor`.

**Dispersion.** Glass with an `abbe` number splits light into colors. A ray traced in every color
splits into three where it first meets such glass, red at the C line (656 nm), green at the d line
(588 nm) and blue at the F line (486 nm), each keeping only its own color of what it then sees;
after that each stays in its color, and any further dispersive glass bends it by that color's
index. Rays from a chromatic lens are already in one color and don't split again. Each split
traces three times as many rays from there on. Ray traces from the eye, so a prism splits what is
seen through it (edges fringe with color) but casts no rainbow on the floor: light reaching a
surface through glass is not traced.

**Textures.** Any material can have a `texture` that gives its color at each point instead of its
`color`. A texture is an object with a `type`:

| `type` | Fields |
|--------|--------|
| `checker` | `colors` (two colors, default black and white), `size` (default `1`), `solid` (default `false`: squares in surface coordinates; `true`: cubes in the scene, which needs no surface coordinates) |
| `marble` | Bands across `axis` (default `[1, 0, 0]`), one each `scale`, bent by turbulence into veins: the ramp's first color is the veins, its last the stone. Default `turbulence` `2.5`; default colors white marble with gray veins |
| `wood` | Rings about the line through `origin` along `axis` (default `[0, 1, 0]`), one each `scale`, wobbled by noise; each ring runs through the ramp, its last color the dark edge. Default `turbulence` `0.3`; default colors pine |
| `noise` | The noise itself, for stone, granite or clouds: with `turbulence` `0` (the default), smooth layered noise through the ramp; with more, `turbulence` times the turbulence function, which has sharp creases. Default colors black to white |
| `image` | `file` (required: a PNG, a PPM, P6 or P3, or a Ray raw image, relative to the scene file), `scale` (`[u, v]` repeats, default `[1, 1]`), `offset` (`[u, v]`, default `[0, 0]`), `repeat` (default `true`; `false` stretches the edge pixels), `srgb` (default `false`: a byte is the color /255, matching the default color model; `true` decodes the sRGB curve) |

`marble`, `wood` and `noise` are solid: they fill space, so an object looks carved from a block and
needs no surface coordinates. They share these fields: `colors` (a ramp of two or more colors,
evenly spaced and blended between), `scale` (default `1`: the size of the pattern in scene units),
`origin` (default `[0, 0, 0]`: where the pattern is centered), `turbulence`, `octaves` (default
`6`, from 1 to 12: layers of noise, each twice as fine and half as strong), and `seed` (default
`0`: each seed is a different block, and the same seed always the same one). The noise is Perlin's
gradient noise. The pattern stays put in the scene, so moving an object moves it through the block.

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

PNG images can be any kind the standard allows: grayscale, RGB, palette, grayscale with alpha or
RGBA, at 1, 2, 4, 8 or 16 bits a channel, interlaced or not. Low bit depths are scaled up to the
full range, 16-bit channels are rounded to 8 bits, and alpha is ignored (the color is used as it
is). Ray reads them with its own decoder, so nothing else is needed; a damaged file is reported
when the scene loads. `tools/make_textures.py` makes the brick images and the sky in `textures/`.

**Bump and normal maps.** An object's `bump` tilts the normal it is shaded with, so light and
highlights fall as if the surface were dented, grooved or rough, while rays still hit the smooth
shape and its outline stays smooth. It belongs to the object, so every material on it (diffuse,
specular, reflection and refraction) sees the same bumps, and a mesh's faces share the mesh's.

| `bump` field | Default | |
|--------------|---------|-|
| `texture` | required | Any texture (as for a material): what the bumps are read from |
| `mode` | `"height"` | `"height"`: the texture is a height field, each point's height the average of its red, green and blue; `"normal"`: the texture is a tangent-space normal map |
| `depth` | `0.05` | Height maps only: how high, in scene units, a texture value of 1 stands. Negative turns the bumps into dents |
| `strength` | `1` | Normal maps only, 0 to 100: how far the map's tilt is followed (0 is flat) |
| `step` | `0.001` | How far either side of a point, in scene units, the texture is looked up to find the slope (height maps) or the surface's u and v directions (normal maps) |

A height map tilts the normal down the slope of its heights. Solid textures (`marble`, `wood`,
`noise`) need no surface coordinates, so they bump any object: a little `noise` makes hammered
metal or orange peel, and an object's own wood or marble texture as its bump makes the grain
stand out. A normal map's red, green and blue, each 0 to 1, are the normal's parts (2c − 1) along
the surface's u direction, its v direction and straight out: the common OpenGL convention, flat at
(0.5, 0.5, 1), with green toward increasing v (up the image). The u and v directions come from the
object's surface coordinates (above), so it needs them. A tilted normal that would face away from
the ray, as at the rim of a sphere, is passed over for the smooth one, so a surface is never lit
from behind.

**Lights.**

| `type` | Fields |
|--------|--------|
| `point` | `position` (required), `color` (required). A point: sharp shadows |
| `sphereLight` | `position` (required): its center; `color` (required); `radius` (default `1`); `samples` (default `1`, up to 256). A glowing ball: soft shadows |
| `rectangleLight` | `position` (required): its center; `color` (required); `edge1` and `edge2` (required): its sides, as vectors; `samples` (default `1`, up to 256). A glowing panel, shining only to the side `edge1` × `edge2` points to: soft shadows |
| `spot` | `position` (required); `direction` (required): which way it points; `color` (required); `angle` (default `30`): degrees from its axis to the edge of its cone, more than 0 and less than 90; `falloff` (default `5`): degrees inside the edge over which it fades (a smoothstep), from 0 (a hard edge) to the angle; `radius` (default `0`) and `samples` (default `1`, up to 256). A light in a cone; with a radius it is a disc (seen from each point as a sphere light is), with soft shadows |
| `directional` | `direction` (required): the way the light travels; `color` (required); `angle` (default `0`): its angular diameter in degrees, less than 90 (the sun is about 0.53); `samples` (default `1`, up to 256). Sunlight: the same light from the same direction everywhere, its shadows the size of what casts them; with an angle, soft shadows that blur the farther they fall. It has no position |

An area light (`sphereLight`, `rectangleLight`, and a `spot` with a radius or a `directional` with
an angle) shades each point from `samples` of its points, each with its own shadow ray and an even
share of its `color`; a sphere light's and a spot's points are on the disc it shows the point, and
a sun's on its disc in the sky. Like a point light, every light gives the same light at every
distance and the camera doesn't see it. A shadow ray toward the sun (or the sky) goes 10⁶ scene
units. Glass casts shadows as if it were opaque, as before.

**`environment`.** What a ray that leaves the scene sees, instead of the flat `background`: camera
rays that miss everything, and reflected and refracted rays that escape, so it shows behind the
scene, in mirrors and through glass. Up is +y.

| Field | Default | |
|-------|---------|-|
| `type` | required | `"sky"` or `"image"` |
| `zenith`, `horizon`, `ground` | `[0.25, 0.45, 0.85]`, `[0.85, 0.9, 1]`, `[0.3, 0.28, 0.25]` | Sky only: the colors overhead, at the horizon and below it |
| `exponent` | `0.5` | Sky only: the colors blend by the height of the direction (its y, from −1 to 1) to this power; smaller keeps the horizon's color in a thinner band |
| `file` | required | Image only: an equirectangular (latitude-longitude) picture of the whole surroundings, twice as wide as it is tall: a Radiance HDR (`.hdr`, RGBE, flat or run-length encoded, stored `-Y height +X width`), PNG, PPM or Ray raw image. Its top row is straight up, its bottom straight down, its middle lies along +z, and +x is a quarter of the way to its right |
| `rotation` | `0` | Image only: degrees to turn it about +y (toward +x) |
| `srgb` | `false` | Image only: decode an 8-bit image's bytes from the sRGB curve (HDR colors are always linear, and may be brighter than 1) |
| `intensity` | `1` | Scales its colors |
| `samples` | `0` | How many shadow rays a point sends toward the sky; 0 (the default): the environment is only seen, and lights nothing |

With `samples`, the environment also lights diffuse surfaces (image-based lighting). The light a
surface would get from the whole sky in front of it, for every way it could face, is worked out
once when the scene loads (into a 32 × 16 map), so it is smooth, without grain, even from a sky
whose sun is far brighter than white. Then each point a ray hits sends `samples` shadow rays toward
the sky, spread by the cosine, and the share that get out scales that light, so corners, crevices
and the ground under things are darker. Shiny materials (specular, reflective, transparent) see the
sky in their reflections instead. It costs `samples` shadow rays at every point a ray hits, and like
area lights it needs many rays a pixel (a stratified sampler) to come out smooth. A sun in the image
is light too, but a small one adds little: give the scene a `directional` light along it for its
sunlight and sharp shadows (the sun in `textures/sky.hdr`, made by `tools/make_textures.py`, is 38°
up, turned 50° from +z toward −x, before any rotation). The image travels inside a scene copy sent to a task
(as base64 `data`, the file's bytes), so networked engines need no files; a saved scene carries it
in its archive.

**Rough surfaces, area lights and the sky's light need many rays a pixel.** Each ray picks one
direction through a rough surface, and one point on each area light (or `samples` points), from where it falls in its
pixel, so a sampler that spreads many rays evenly over each pixel (`"stratified"` or `"adaptive"`)
spreads them evenly over the blur and the light too; with one ray a pixel they come out grainy. A
ray made at a rough surface picks again from a point hashed from its parent's, so the bounces
after the first are independent of it. None of this is random: the same scene always gives the
same image, and networked engines give exactly the same as threads.

## Format history

- **Version 1** (before April 1997) was binary. It stored every floating-point value as two 4-byte
  integers, a whole part and a fraction of 2³¹, and read objects until the end of the file.
- **Version 2** (1997–1999) was binary too, but stored floating-point values as `%.18e` text and
  wrote an object count. For a few weeks files had the version-2 header but still used the old
  numbers; `CScene::Load` tries both.
- **Version 3** (2026) is JSON, with the same file extension. It adds the background color, the
  camera's sampler and color model, triangle sidedness and smoothing, meshes, and cylinders, cones
  and boxes; triangles no
  longer store normals. The binary versions are no longer read. Later additions are optional
  fields that older readers would refuse but that change nothing when left out, such as an
  object's `bump` (2026-alpha-0.8), and the `environment` and the `spot` and `directional` lights
  (2026-alpha-0.9), and `csg` objects, a transparent material's `abbe` and the camera's `clamp`
  (2026-alpha-0.10).

`renders/` holds output files from 1997. `scan_test.raw` there is a partial trace from that time
(a few scan lines over black), not a reference image.
