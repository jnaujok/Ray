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
| `chromatic_test.ray` | | `scan_test` through an uncorrected BK7 singlet (stop behind it), whose colors focus apart: chromatic aberration (traced spectrally), plus the softness of one simple lens |
| `texture_test.ray` | | Textures: a checkered floor, `textures/uv_grid.ppm` on a wall and a sphere, and a solid checker sphere |
| `primitives_test.ray` | | Cylinders, cones and boxes: a capped cylinder, an open tube lying on its side (lit only where light gets in), a cone, a truncated cone and a turned cube with `textures/uv_grid.ppm`, and a turned glass box |
| `torture_test.ray` | | Everything at once, at 1024×768: the teapot (polished marble, the Utah teapot's own 32 Bezier patches traced exactly) among a mirror sphere, a frosted glass sphere, a textured sphere, a solid-checker column, an open brushed and hammered gold tube, a hammered cone, a textured truncated cone, a glass cube with green swirls inside (its `attenuationColor` a `marble` texture), a brick box (a PNG texture with a PNG normal map), a textured triangle, a flint glass prism that splits what is seen through it into colors, and a die with drilled pips (both csgs), a gold torus (the `metal` material) standing on its rim, a purple cluster of four metaballs (a `blob`) behind the mirror sphere, the word "RAY" in beveled red letters (Lato Bold) standing on top of the mirror frame, on a reflective checkered floor before a mirror wall frosted in patches (its `roughness` a `cells` texture) in a wooden frame whose grain is bumped (the `walnut` preset, turned along the frame), a malachite cone (a preset), lit by a sphere light, a point light, a rectangle light, a soft-edged spot light on the teapot with a `fresnel` beam that throws a dappled `caustics` pattern (its `filter`) and a low sun with an angle whose `intensity` is a `noise` texture, so cloud shadows lie across the floor, under the HDR sky `textures/sky.hdr` (turned so its sun is over the key light), which shows in the mirrors and glass and lights the diffuse surfaces, through the double Gauss stopped down to f/8 (200 mm a unit, focused at 18, so everything from the front row to the mirror is sharp) with a 5×5 stratified sampler (25 rays a pixel, each traced at four single wavelengths: spectral dispersion), each sample clamped at 1.5, with one bounce of indirect light (one ray a sample aimed at each of the two objects within 4 units that fill the most of the view) and a light haze that thins with height, so the far floor and the horizon fade into it |
| `solid_textures_test.ray` | | Solid textures and absorbing glass: a white marble sphere, a green marble cube, a wooden log and a wooden floor before a granite wall, three green glass slabs 0.15, 0.5 and 1.2 thick (the thicker, the darker), and an amber glass ball |
| `bump_test.ray` | | Bump and normal maps, and PNG textures: a brick wall (a PNG color texture and an interlaced PNG normal map) lit from the side, a hammered copper ball and an orange-peel ball (`noise` height maps), a box embossed with bricks by a 16-bit grayscale PNG height map, and a wood floor bumped by its own grain, with a 3×3 stratified sampler |
| `lights_test.ray` | | Spot and directional lights: a warm spot with a hard edge on a box, a white spot with a soft edge on a ball, a blue spot with a soft edge that is also a disc (`radius`), so the cone's shadows are soft, and a dim low sun with an angle of 4°, whose shadows blur the farther they fall, with a 4×4 stratified sampler |
| `environment_test.ray` | | An environment map: the HDR sky `textures/sky.hdr` behind the scene, in a mirror ball and through a glass one, lighting a white ball, a small red one and a checkered floor (`samples` 8, so the floor darkens under the balls), with a sun whose direction matches the sky's, with a 4×4 stratified sampler |
| `bounce_test.ray` | | Indirect light: a box room with a red wall on the left and a green one on the right, a white block and a white ball, lit only by a small rectangle light under the ceiling. One bounce of light gathered from the nearby objects (`"objects"`, 9 rays an object) bleeds red and green onto the white surfaces and fills the shadows and the ceiling, with a 3×3 stratified sampler at 160×120 |
| `bounce_hemisphere_test.ray` | | `bounce_test` gathered with the `"hemisphere"` method instead, at the same 54 rays a point (9 an object for the 6 objects in reach), for contrast: in a closed room none of its rays escape, so it is smoother for the same rays |
| `haze_test.ray` | | Haze: an avenue of colored columns and balls receding along a checkered floor under a sky, fading into ground mist that thins with height (`density` 0.16, `heightFalloff` 0.7), lit by two point lights (one far down the avenue) and a sun, with a 2×2 stratified sampler at 160×120 |
| `materials_test.ray` | | The material presets: thirty spheres before a gray wall, one of each finish in the library (`{ "preset": name }`), in its order: the marbles, travertine, the granites, slate, sandstone, basalt, sand, concrete, terrazzo and terracotta, the seven woods, jade, amber, obsidian, malachite and lapis lazuli, the brushed metals and rust, lit by two point lights with the `corners` sampler |
| `projector_test.ray` | | Light profiles and filters: six spot lights on a wall, one of each beam `profile` (smoothstep, linear, cosine, gaussian, fresnel and a table with rings), a spot that projects `textures/uv_grid.ppm` onto the floor like a slide (its `filter`), a black marble ball in its beam, and a warm point light near the floor that falls off with the square of distance (`attenuation`) |
| `underwater_test.ray` | | Underwater: blue-green haze, a rippled sand floor (the `sand` preset and a stretched noise bump), a basalt boulder, a terracotta block, a jade ball and a travertine column, and sunlight that throws the `caustics` preset over everything (its `filter`) |
| `soft_test.ray` | | Rough materials and area lights: three glass panes before a striped poster, clear, lightly frosted (roughness 0.08) and heavily frosted (0.3), a mirror ball and a brushed gold one (0.12), a post and a red ball casting soft shadows from a rectangle light overhead and a sphere light to the right, with a 6×6 stratified sampler |
| `csg_test.ray` | | Constructive solid geometry: a glass lens blank (two balls' intersection), a bowl (a ball with a smaller one and a turned box taken out, blue outside, white inside and clay at the cut rim, by `childMaterials`), a die (a rounded cube, itself the intersection of a cube and a ball, with six pips drilled in and painted black) and a hammered gold capsule (a cylinder and two balls, with a bump map), with a 3×3 stratified sampler |
| `models_test.ray` | | Model files: a puzzle cube from a VisCAM-colored STL, a gem from a Magics-colored STL (four faces with their own colors, four with the model's), both shown by the `faceColors` texture, and a ring from an OBJ file of quads with texture coordinates and normals, wearing an image texture, each placed by `fitSize` or `scale`, `rotation`, `align` and `translation`, with a 2×2 stratified sampler |
| `maps_test.ray` | | Texture maps: a glass ball frosted in patches (its `roughness` a `cells` texture), a mirror ball polished in patches (`noise` roughness), a glass ball whose index wanders from 1.1 to 1.9, a lace panel of clear glass and blue paint in a solid checker (each material's `influence` a texture), a floor bumped by noise whose `depth` varies by `cells`, and a spot light whose `tint` is a `marble` texture and whose `intensity` is `cells`, both read at the point it lights; a 3×3 stratified sampler |
| `volume_test.ray` | | Textures inside glass (`attenuationColor` as a texture): art glass with blue and red `marble` swirls, smoky quartz (`noise`), amber with dark flecks (`cells`), and gray veins floating in a clear block (`marble`); a 3×3 stratified sampler |
| `benchy_test.ray` | | 3DBenchy (`models/3DBenchy.stl`, 225,706 triangles) printed in orange plastic, close up on its bow, its 0.1 mm layer lines a `layers` texture for both its color and its bump map, with a 3×3 stratified sampler |
| `shapes_test.ray` | | The shapes of 0.16: a red torus tilted toward the camera, a gold disc standing up, a blue annulus on the floor, an open mirror-polished paraboloid (a dish), a capped green hyperboloid, a purple superquadric that is nearly a rounded cube (`e1` and `e2` 0.35), an orange pinched star (both 2.6) and a rounded box, with the `corners` sampler |
| `text_test.ray` | | Text from a TrueType font (`fonts/Lato-Bold.ttf`): "Ray 0.16" in beveled gold letters, "Typeset in / Lato Bold" in two centered lines, turned, and "CSG" carved out of a rounded slab (a csg difference), with the `corners` sampler |
| `patches_test.ray` | | Bezier patches: the Utah teapot as Martin Newell's 32 bicubic patches (`models/teapot.bpt`, traced exactly) on the right, beside the mesh teapot made from them on the left, in the same red glaze, with the `corners` sampler |
| `nurbs_test.ray` | | B-spline and NURBS surfaces: a gold torus made of rational quadratic circles both ways (a `bspline` with weights) in front of the analytic `torus` of the same size, and a bicubic B-spline sheet, gently waved, with `textures/uv_grid.ppm` running across all its patches and a fine noise bump, with the `corners` sampler |
| `lathe_test.ray` | | Solids of revolution: a black pawn and a white rook (profiles of lines and curves), a black queen (a smooth profile through points), a glass goblet, an open terracotta vase (the `terracotta` preset; an open profile, a shell) and an orange with a quarter cut away (`sweep` 270), with the `corners` sampler |
| `extrusion_test.ray` | | Extrusions and sweeps: a gold star tapered to a point, a blue square column twisted 120 degrees, a red frame with a round hole beveled front and back, a steel spring and a green closed loop of tube (sweeps along smooth paths) and a brown zigzag rail with a square cross-section, with the `corners` sampler |
| `implicit_test.ray` | | Implicit surfaces: a sphere from its formula beside the sphere itself, a gyroid's labyrinth cut down to a ball by a csg, and Taubin's heart, with the `corners` sampler |
| `blob_test.ray` | | Metaballs: five blue balls melted together, and an orange ball dented by two of negative strength, with the `corners` sampler |
| `fractal_test.ray` | | Fractals by sphere tracing: a mandelbulb, a Menger sponge and a Sierpinski tetrahedron, with the `corners` sampler |
| `apollo_lenses_test.ray` | | The Hasselblad Electric Camera (`body`): the 80 mm Planar at f/4 on Ektachrome MS, looking at three balls and a checkered target in sunlight, with a 3×3 stratified sampler |
| `film_test.ray` | | Kodak Ektachrome MS (`film` color model) on a 20 mm piece of film: gray steps a stop apart from 0.02 to 1.28, red, green and blue balls, and a far overexposed lamp whose light spreads into a red-orange halo (halation), with grain |
| `reseau_test.ray` | | The Hasselblad Data Camera's Réseau crosses (drawn wider than the plate's own, and with more spread, so they show at this size): a bright white ball blooms over the cross on it, while the cross on a gray ball stays sharp |
| `tranquility_test.ray` | | Tranquility Base: a lunar module built from boxes, a rounded box and cylinders (gold foil on four legs) beside a flag, on `regolith` in low sunlight (a 0.53° sun) under a black sky, shot with the Data Camera, its 60 mm Biogon, Réseau plate and Ektachrome MS; an imitation of the Apollo 11 views of the LM, not a recovered camera station |
| `chess_dof_test.ray` | | Depth of field: a full chessboard in the Najdorf Sicilian, English Attack, after 1.e4 c5 2.Nf3 d6 3.d4 cxd4 4.Nxd4 Nf6 5.Nc3 a6 6.Be3 e5 7.Nb3 Be6 8.f3 Be7 9.Qd2 O-O 10.O-O-O Nbd7 11.g4 b5 (FEN `r2q1rk1/3nbppp/p2pbn2/1p2p3/4P1P1/1NN1BP2/PPPQ3P/2KR1B1R w - - 0 12`), played out by python-chess, the pieces Poly Haven's Chess Set (CC0) read from its glTF (`node` picks each piece), polished boxwood and ebony on an inlaid board, through the double Gauss wide open (f/2.02) focused on Black's pawn on e5, with twelve small lights behind the board that blur into bokeh discs; 160×120, a 3×3 stratified sampler. Written by `tools/make_showcase_scenes.py` |
| `as11_40_5903_test.ray` | | A recreation of NASA photograph AS11-40-5903: Buzz Aldrin (NASA's public-domain astronaut model, its arms lowered by `tools/pose_astronaut.py`) in a white suit with a gold `metal` visor that reflects Neil Armstrong (a second copy, behind the camera) and the lunar module (NASA's model, its glTF base colors through `faceColors`), the Solar Wind Composition foil and the flag (from ALSJ dimensions), on regolith under a 15° sun and a black sky, with Earth small and high; shot on the Data Camera with its 60 mm Biogon, Réseau plate and Ektachrome MS, one bounce of indirect light; 128×128. Written by `tools/make_showcase_scenes.py`. The camera was placed by eye against the photograph, not from a recovered camera station |
| `blend_test.ray` | | Models read straight from Blender `.blend` files: the red and blue cube of `models/blend_legacy_zst.blend` (Blender 2.93's mesh layout, Zstandard-compressed, its one object named by `node`) and the gold pentagon with its tetrahedron child from `models/blend_modern_gz.blend` (Blender 4.2's attribute layout, gzip-compressed), colored by their materials through `faceColors` |
| `prism_test.ray` | | Dispersion: a flint glass prism (an intersection of three turned boxes) and a flint glass ball (index 1.62, Abbe number 20) before white bars on a black wall, which they fan out into a spectrum (spectral dispersion), with a 2×2 stratified sampler |
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
| `haze` | none | An atmosphere: haze, fog or ground mist that what is far away fades into, and that dims light on its way to a surface. Below |
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
| `indirect` | none (off) | Light bounced between surfaces, below |
| `reserved1997` | | Settings the 1997 code defined but never implemented (anti-aliasing, caustics, focal blur, haze, and so on), kept so nothing is lost. Only the ones that differ from their defaults are written. The tracer ignores them. |

**`settings.indirect`**: light bounced between surfaces, a "poor man's radiosity": colors bleed
from one surface onto another, and shadows and corners get fill light instead of going black. It
works at every point a ray hits on a surface with a `diffuse` material. The light gathered there,
times `strength`, lights the point's diffuse materials as a light straight along the normal.
Whatever a gathering ray meets sends back only the light it scatters diffusely: its diffuse
materials lit by the lights (with shadows) and the sky, what glows (`ambient`), and, while
`bounces` are left, its own gathered light; reflections, highlights and refraction don't count. All
of it is worked out from the ray alone (its sample point in its pixel), with no cache, so a
stratified sampler smooths it and every thread and networked engine gives exactly the same image.
Gathered light respects the camera's `clamp`. Two methods:

- **`"objects"`** (the default): each object near the point is treated as a light. The scene's
  tree finds the objects whose bounds come within `distance`; of those, the `objects` that fill the
  most of the point's view (an object's share of it is the solid angle of its bounding sphere; a
  plane's is the disc of it within `distance`) each get `samples` rays aimed evenly over the cone
  of directions it fills. A ray counts only if that object is the first thing it meets, so no
  direction counts twice, and it counts by the cosine at the point and the solid angle it stands
  for, so the answer is right whatever the number of rays. An object the point is inside the bounds
  of (the room it stands in, a mesh it is part of) is gathered from over the whole half above the
  point instead, with four times `samples` rays (since 2026-alpha-0.21: spread that thin, one ray
  showed as speckles), and a closed object (a sphere, a box) lights itself when seen from inside.
  Cost: at most `objects` × `samples` gathering rays at each diffuse hit (four times that for an
  object around the point), each shading what it meets with direct light (more with `bounces`
  above 1), so it stays predictable however big the scene.
- **`"hemisphere"`**: `samples` rays spread by the cosine over the whole half above the point, each
  reaching `distance`. Nothing nearby is missed, but most rays in an open scene hit nothing. Cost:
  `samples` gathering rays at each diffuse hit. A ray that hits nothing in reach brings nothing:
  distant light is the sky's and ambient's to give.

With either, each sample gathers with `samples` rays (per object) at the first bounce and one at
every bounce after it, so the cost grows with `bounces` instead of multiplying; a scene with a 5×5
sampler and `samples` 1 still gathers 25 directions a pixel. One bounce is usually enough.

A sample's rays start from its sample point (where it fell in its pixel, as a rough mirror's
direction does) and spread from there by the R2 sequence, so over a stratified sampler's samples
they cover each object's cone evenly: `samples` 9 with a 3×3 sampler gathers 81 directions an
object a pixel. (2026-alpha-0.14.1 measured an even grid of rays, one a cell, laid out by the
sample's index: it was 10 to 20% noisier than this.) Each gathered ray is limited by the camera's
`clamp`, as samples are.

How many rays: `tools/bounce_sweep.py` measures noise and time against `samples`. On
`bounce_test` (a closed room) the noise falls about as one over the square root of the rays and
the time grows with them: 1 ray an object is grainy, 9 halves the noise of 2, and past 25 each
step buys little. In a closed room `"hemisphere"` is several times smoother for the same rays a
point, since none of its rays escape; `"objects"` pays off in open scenes, where most of the
hemisphere's rays find nothing.

| Field | Default | |
|-------|---------|-|
| `method` | `"objects"` | `"objects"` or `"hemisphere"`, above |
| `bounces` | required | How many times light may bounce, 0 (off) to 8 |
| `samples` | `4` | Gathering rays at the first bounce (for `"objects"`, per object), 1 to 256 |
| `objects` | `8` | For `"objects"`: the most objects one point gathers from, 1 to 64 |
| `distance` | `10` | How near an object must be (`"objects"`), or how far a ray reaches (`"hemisphere"`), in scene units (more than 0, at most 1,000,000) |
| `strength` | `1` | A scale on the gathered light, 0 to 100 |

**`haze`**

A ray is blended toward the haze's `color` by how much light gets through the haze along it, the
transmittance `exp( -∫ density )`: as far as what it hits, or 1,000,000 units for a ray that leaves
the scene, so the sky is hazed too. Light from a light to a surface is dimmed by the same
transmittance along its path. Sunlight (a `directional` light) comes from outside the scene, so
in haze that is the same everywhere it isn't dimmed (its path would never end); with a
`heightFalloff` it is dimmed over its whole path up and out of the haze. The integral has a closed
form, so haze costs almost nothing.

| Field | Default | |
|-------|---------|-|
| `color` | `[0.7, 0.75, 0.8]` | What the haze looks like, and what far away things fade into |
| `density` | required | How thick it is, per scene unit, at `baseHeight` (0 to 1,000,000) |
| `heightFalloff` | `0` | With 0 the haze is the same everywhere. Above 0 it thins by e for every 1 / `heightFalloff` units above `baseHeight` (and thickens below it), like mist lying on the ground. Up is +y |
| `baseHeight` | `0` | The height where the haze is `density` thick |

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
| `body` | none | A camera body: `"hasselblad-hec"` (the Electric Camera: 80 mm Planar, 55 mm format), `"hasselblad-hdc"` (the Data Camera: 60 mm Biogon with its Réseau plate, 53 mm format, and the crosses) or `"hasselblad-lswc"` (the Super-Wide-Angle Camera: 38 mm Biogon). It fills in what the camera doesn't give: the lens (a `lens` without a `prescription` or `surfaces` keeps the body's and sets the rest), its `sensorWidth`, a `film` color model (Ektachrome MS, 1/250 s at f/11, or f/8 for the HEC) and the Réseau plate. A saved scene holds the camera the body made, not the body's name |
| `reseau` | none | A Réseau plate's crosses, printed by the plate on every frame: `count` (default `5` on a side), `spacing` (`10` mm), `size` (`1.2` mm across each cross) and `lineWidth` (`0.05` mm). No light reaches the film under a cross, so the crosses come out black, but the film's spread of light (see `film`) fills them in where the picture is overexposed, as it did on the Apollo frames. Needs a lens (the crosses are placed in mm on the film). The plate's glass is part of the `biogon-60mm-reseau` prescription |
| `colorModel` | `"clip"` | How pixel colors become bytes: `"clip"` (clip at 1, as in 1997), `"gamma"`, `"srgb"`, `"reinhard"`, `"hdr"`, or `"film"` (a Kodak film stock, below) |
| `dispersion` | `"rgb"` | How white light splits where it disperses (glass with an `abbe` number, a chromatic lens): `"rgb"`, into red, green and blue rays, or `"spectral"`, into four single wavelengths weighed by the CIE color matching functions, which smear into a continuous rainbow over a pixel's samples (see Dispersion, below). Scenes without dispersion render the same either way |
| `clamp` | `0` (off) | The brightest any one sample's red, green or blue may be before the sampler averages it in. A rare ray that finds a light through rough glass, off a rough mirror or from an area light can be far brighter than its pixel's other rays and show as a lone bright speck (a "firefly"); a clamp of 1.5 to 2 removes them, at the cost of a little of the brightest highlights. Can't be negative |

A sampler or color model is a name, or an object with a `type` and its settings:
`{ "type": "random", "samples": 9, "seed": 0 }` (N from 1 to 1024, default 4; the same seed always
gives the same image), `{ "type": "stratified", "grid": 5 }` (n from 1 to 32, default 4),
`{ "type": "adaptive", "min": 9, "max": 64, "threshold": 0.01 }` (each pixel gets at least `min`
rays, then more until the standard error of its brightness, on the scale where 1 is white, is under
`threshold`, or it has `max`; rays follow a low-discrepancy sequence, reproducible like the others;
two opt-in refinements, added in 2026-alpha-0.20.1: `"confidence": 0.95` (default `0`, off; else 0.5
to 0.999) stops once Student's t for that confidence times the standard error is under `threshold`,
so a few rays that happen to agree must agree much more closely (t is 3.18 at 4 rays; keep `min` at
about 16 all the same, since rays that agree exactly have no spread for t to widen), and `"channels": "rgb"` (default `"brightness"`) measures red, green and blue apart
and uses the worst, so a pixel that changes color but not brightness keeps sampling),
`{ "type": "gamma", "gamma": 2.2 }`, or
`{ "type": "reinhard", "exposure": 1, "white": 4 }`
(`exposure` scales the colors first; with a `white` luminance, that and anything brighter maps to
white), or `{ "type": "hdr", "exposure": 1 }` (high dynamic range: the colors scaled by
`exposure` go unclipped into `raythrower --hdr` files; 8-bit files get them clipped and
sRGB-encoded).

**Film.** `{ "type": "film", "stock": "ektachrome-ms", "shutter": 0.004, "fStop": 11 }` develops the
image as a real film stock: each layer (one for black and white, three for color) is exposed by a
mix of the red, green and blue reaching it, its density follows the stock's characteristic curve (a
positive: a slide, or a print of a negative), and the slide is shown, sRGB-encoded, with the clear
base as white. A linear value v gets exposure v × 256 × `shutter` × ISO / `fStop`², so by the
"sunny 16" rule (f/16 at 1/ISO seconds) a value of 0.18 develops to the stock's mid gray. A lens
keeps the image as bright as a pinhole whatever its own stop, so the film's `fStop` sets the
exposure. Once the image is traced the whole of it is developed again with the film's grain
(random in density, the same every run, its RMS growing with density and scaled from the stock's
48 µm figure to a pixel's width) and the spread of light in the emulsion: irradiation (scatter
between the grains) and halation (light reflected back from the base, reddest in color film), each a
share of the light carried off in a Gaussian. A bright area overexposes its surroundings by the
same share of its light, so the more overexposed it is, the farther its glow reaches past the
curve's shoulder. Grain and spread are done by whatever made the image (the master, for threads
and networked engines alike), so they need the whole image; HDR files get the linear colors.

| Field | Default | |
|-------|---------|-|
| `stock` | `"ektachrome-ms"` | `"ektachrome-ms"` (Kodak Ektachrome MS, SO-368, ISO 64 color reversal: Apollo's workhorse), `"ektachrome-ef"` (SO-168, ISO 160 color reversal), `"panatomic-x"` (3400, ISO 80 fine-grain black and white) or `"high-speed-2485"` (2485, ISO 6000 black and white) |
| `iso` | the stock's | The speed it is exposed (or pushed) at |
| `shutter` | `0.004` (1/250 s) | Seconds |
| `fStop` | `11` | The exposure's f-stop |
| `width` | `55` | The image's width on the film, in mm (the grain and spread are in mm on the film) |
| `grain`, `spread` | `1`, `1` | Scales on the stock's grain and on its irradiation and halation; `0` turns them off |
| `seed` | `0` | Picks the grain |

The stocks are approximations: their speeds and kinds are Kodak's, but their curves, color
response, grain and spread are representative values, not measured data, and their spectral
sensitivity is a mix of the renderer's red, green and blue.

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
| `prescription` | | A built-in lens: `"double-gauss-50mm"` (50 mm f/2, US patent 2,673,491, as tabulated by Kolb et al., 1995) or one of the Apollo Zeiss lenses below |
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
of the glass that follows (leave it out for no dispersion). Instead of `ior` and `abbe`, `glass`
names a crystal traced with its own Sellmeier formula: `"fused-silica"` or `"calcium-fluoride"`
(Malitson, 1965 and 1963), which disperse too little, and too unevenly, for a Cauchy fit.

**The Apollo lenses.** The Carl Zeiss lenses of the Apollo Hasselblads, at their nominal focal
lengths, written by `tools/apollo_lenses.py`, which says where each comes from and measures them:

| `prescription` | Lens | Source |
|----------------|------|--------|
| `biogon-38mm` | Biogon 38 mm f/4.5 (the SWC's) | US patent 2,721,499 (L. Bertele), Example 2: 8 elements in 5 groups, f/4.5, over 90° |
| `biogon-60mm` | Biogon 60 mm f/5.6 (the Data Camera's) | A reconstruction: no prescription was published, so the same Biogon at the 61.1 mm focal length Zeiss gave for the flight lens |
| `biogon-60mm-reseau` | the same with the Data Camera's Réseau plate | The plate is 3 mm of BK7-like glass (an assumption) whose back face lies 0.02 mm in front of the film at infinity |
| `planar-80mm` | Planar 80 mm f/2.8 | US patent 2,724,994 (G. Lange, Zeiss), Example I: the 5-element, 4-group Planar formula; the Hasselblad lens was a later 7-element version |
| `planar-100mm` | Planar 100 mm f/3.5 | A family member: Example II of the same patent at 100 mm, opening to f/3.5 |
| `uv-sonnar-105mm` | UV-Sonnar 105 mm f/4.3 | A reconstruction: seven air-spaced singlets of fused silica and calcium fluoride, four before the stop and three after, optimized for the visible band |
| `sonnar-250mm` | Sonnar 250 mm f/5.6 | A reconstruction: 4 elements in 3 groups (a collective lens, a cemented doublet, the stop, a dispersive meniscus), about 0.91 as long as its focal length |
| `tele-tessar-500mm` | Tele-Tessar 500 mm f/8 | A reconstruction: 5 elements in 3 groups (a cemented doublet, a meniscus, the stop and a cemented telephoto doublet), about 0.71 as long as its focal length |

Ray traces visible light only (380–720 nm), so the UV-Sonnar is modeled in the visible band.

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
| `mesh` | `vertices` (required): points; `faces` (required): three vertex indices each, from 0; `doubleSided` (default `false`); `smooth` (default `false`, or `true` with `normals`); `normals`: one for each vertex; `uvs`: one for each vertex, or `uvMap` (below). Or, instead of `vertices`, `faces`, `normals` and `uvs`, a model `file` (an STL or OBJ file) and its placement: see *Model files* below |
| `csg` | `operation` (required): `"union"`, `"intersection"` or `"difference"`; `objects` (required): two or more closed shapes; `childMaterials` (default `false`) |
| `torus` | `center` (required); `majorRadius` (required): from the center to the middle of the tube; `minorRadius` (required): the tube's radius; `rotation` |
| `disc` | `center` (required); `radius` (required); `innerRadius` (default `0`, less than `radius`): a hole in the middle; `rotation` |
| `annulus` | a `disc` whose `innerRadius` (required) is more than 0 |
| `paraboloid` | `center` (required): the bowl's lowest point; `radius` (required): its rim's; `height` (required); `capped` (default `false`); `rotation` |
| `hyperboloid` | `center` (required); `radius` (required): at its waist; `topRadius` (required, at least `radius`): at both ends; `height` (required); `capped` (default `false`); `rotation` |
| `superquadric` | `center` (required); `radius` (required): one number, or `[x, y, z]`; `e1` and `e2` (default `1`, from 0.05 to 10): its shape from pole to pole and around; `rotation` |
| `roundedBox` | `center` (required); `size` (required): one number for a cube, or `[x, y, z]`; `radius` (required): the edges' and corners', more than 0 and at most half the smallest size; `rotation` |
| `text` | `string` (required, UTF-8; `\n` starts a new line); `font` (required): a TrueType font file; `size` (default `1`): the em in scene units; `depth` (default `0.2`); `bevel` (default `0`, less than `depth`); `align` (default `"left"`, or `"center"` or `"right"`); `lineSpacing` (default `1`); `kerning` (default `true`); `position` (default `[0, 0, 0]`); `rotation` |
| `lathe` | `profile` (required): a path in r and y (see below), r at least 0; `smooth` (default `false`): a smooth curve through the profile's points; `capped` (default `false`): closes open ends with flat discs in to the axis; `sweep` (default `360`, more than 0): degrees around, from +x toward +z; `center` (required); `rotation` |
| `extrusion` | `outline` (one) or `outlines` (several; holes worked out from how they nest): closed paths (see below); `smooth` (default `false`); `depth` (default `1`); `bevel`, `backBevel` (default `0`; together less than `depth`); `taper` (default `1`, more than 0): the scale at the back; `twist` (default `0`): degrees; `position` (default `[0, 0, 0]`); `rotation` |
| `sweep` | `path` (required): two or more points `[x, y, z]` in the scene; `smooth` (default `true`): a curve through them (else straight from point to point); `closed` (default `false`); `radius` (default `0.1`) or `outline`: a closed path (see below) as the cross-section; `segments` (default `24`, 3 to 256) around a round tube; `steps` (default `12`, 1 to 256) along each span of a smooth path; `capped` (default `true`) |
| `bezier` | `patches` (required, unless `file`): each `{ "points": [...] }` (u varies fastest) or `{ "indices": [...] }` into the shape's own `points`, with `degree` (`[u, v]`, from 1 to 15; needed unless the points are a square number: 16 is bicubic) and `uv` (default `[0, 0, 1, 1]`); or `file`: a `.bpt` file of patches; `points`: the shared points for `indices`; a point is `[x, y, z]` or `[x, y, z, weight]`; `scale` (default `1`): a number or `[x, y, z]`; `closed` (default `false`); `center` (default `[0, 0, 0]`); `rotation` |
| `bspline` | `points` (required): rows of points along v, each an array along u, `[x, y, z]` or `[x, y, z, weight]`; `degree` (default `[3, 3]`, or one number for both); `knotsU`, `knotsV` (default clamped and evenly spaced): points in a row (or rows) + degree + 1 numbers, never decreasing; `scale`; `closed`; `center` (default `[0, 0, 0]`); `rotation` |
| `implicit` | `formula` (required): see below; `parameters`: an object of names and numbers; `bounds` (required): half-sizes, a number or `[x, y, z]`; `steps` (default `256`, 4 to 1000000); `lipschitz` (default none); `scale` (default `1`); `center` (required); `rotation` |
| `blob` | `balls` (required): `{ "center", "radius", "strength" }` each (`strength` default `1`); `threshold` (default `0.5`); `scale`; `center` (required); `rotation` |
| `fractal` | `kind` (required): `"mandelbulb"`, `"menger"` or `"sierpinski"`; `iterations`; `power`, `bailout` (mandelbulb only); `epsilon` (default `0.0001`); `maxSteps` (default `256`); `stepFactor` (default `1`); `scale`; `center` (required); `rotation` |

A cylinder or cone points from `base` to `top`, any way. Capped, its ends are closed with flat discs
and it is a solid; with `"capped": false` it is an open shell, seen into from its ends, and it
shadows and reflects itself inside. A cone's radii can't be negative or both 0, and `base` and
`top` can't be the same point. A box is centered on `center`, `size` long along its own axes, which
`rotation` turns from the scene's x, y and z.

**Tori, discs, quadrics, superquadrics and rounded boxes.** These stand on their own +y before
their `rotation` (default `[0, 0, 0]`: degrees about x, then y, then z, as a box's) turns them, and
are centered on `center`:

- A **torus** lies around +y: the circle of `minorRadius` swept around at `majorRadius`. A ray meets
  it where a quartic is zero; the quartic is set up from where the ray enters the torus's bounding
  sphere, so it keeps its precision far from the origin and far from the camera, and each root is
  found by bisection between its turning points. Surface coordinates: u around +y, v around the tube.
- A **disc** (or an **annulus**, with a hole of `innerRadius`) faces +y and is shaded on whichever
  side a ray sees. Surface coordinates: u around, v from the inner edge to the rim.
- A **paraboloid** is a bowl, x² + z² = (radius² / height) y, from its lowest point at the center up
  to its rim. A **hyperboloid** (of one sheet, like a cooling tower) runs `height` along +y, half
  above the center and half below, `radius` wide at its waist and `topRadius` at both ends (equal,
  it is a cylinder). Open (the default), either is a shell seen from both sides; `"capped": true`
  closes its ends with flat discs. Surface coordinates: u around, v from bottom to top; a cap holds
  the unit square.
- A **superquadric** (a superellipsoid) is ( |x/a|^(2/e2) + |z/c|^(2/e2) )^(e2/e1) + |y/b|^(2/e1)
  ≤ 1, with `radius` a, b, c. `e1` shapes it from pole to pole and `e2` around: both 1 is an
  ellipsoid (a sphere for equal radii), near 0 a box with sharp edges, 2 a double cone or an
  octahedron, above 2 a pinched star. With both at most 2 it is convex and a ray finds it exactly,
  grazing rays too; above 2 the ray steps 256 times through its box, which can miss parts thinner
  than a step.
- A **rounded box** is a box of `size` whose edges and corners are rounded to `radius` (half every
  size is a sphere). It is exactly a smaller box grown by the radius, found with no stepping.

The torus, the superquadric, the rounded box, and a capped paraboloid or hyperboloid are solids, so a
csg can use them; discs and open quadrics can't. Bump maps, textures and maps work on all of them.

**Text.** A `text` object sets its `string` in a TrueType `font` (a `.ttf`, or an `.otf` with
TrueType outlines; fonts with CFF outlines and font collections are refused, saying why) and
extrudes it into a closed mesh, so it takes any material, works in a csg (letters carved out of a
slab) and casts and catches shadows. The text lies in its own x–y plane with y up: the first
line's baseline starts at `position` (`"left"`), is centered on it (`"center"`) or ends at it
(`"right"`), and each line below the last by the font's line height times `lineSpacing`. The face is
at z = 0 facing −z, and the letters run `depth` back along +z; then `rotation` turns them about
`position`. `size` is the font's em (a capital letter is about 0.7 of it). Curves are flattened to
within 0.15% of the em, holes found from how a glyph's outlines nest (so either winding works),
and the face cut into triangles; the sides are smooth around curves and sharp at corners of more
than 35°. `bevel` cuts the front edges at 45°, `bevel` deep (a stroke too thin for it is left
square). Kerning comes from the font, its `kern` table or its OpenType GPOS pair kerning, unless
`"kerning": false`. Hinting is ignored. The font file goes into a saved scene's archive, and to
networked engines inside the scene (as `data`), like a model file. Surface coordinates: x and y in
the text's plane, one unit per em. `text_test.ray` uses Lato Bold (`scenes/fonts/`, SIL Open Font
License 1.1).

**Paths.** A lathe's profile, an extrusion's outlines and a sweep's cross-section are paths: arrays
whose entries are points `[x, y]` (a straight line to there), `{ "quadratic": [ [cx, cy], [x, y] ] }`
(a curve to x, y pulled toward the control point) or `{ "cubic": [ [c1x, c1y], [c2x, c2y], [x, y] ] }`
(a curve with two control points). The first entry is where the path starts, so it must be a point.
An outline (an extrusion's, a sweep's) is closed: it runs back to its start. With `"smooth": true`
every entry must be a point, and the path is a smooth curve through them all (uniform Catmull-Rom;
an outline wraps around, a lathe's profile starts and ends at its first and last points). A path is
saved as it was given.

**Lathes.** A `lathe` spins its profile (r the distance from the axis, y the height) around its own
+y, centered on `center` and turned by `rotation` as a box is. It is traced as it is: each piece of
the profile, spun around, meets a ray where a polynomial in the piece's own parameter is zero (of
degree 2 for a line, which makes a cone or a flat ring, up to 6 for a cubic), found by bisection
between its turning points and polished by Newton's method, so curved profiles stay smooth at any
size and nothing slips through where pieces join or at the axis. A profile that starts and ends on
the axis (r = 0) encloses a solid, and so does any profile with `"capped": true`, whose open ends
are closed by flat discs in to the axis; its normals point out of the solid whichever way the
profile runs. An open profile is a shell, shaded on whichever side a ray sees. `sweep` turns the
profile only part way round, from +x toward +z; a solid is then closed by flat faces at both ends of
the cut. A solid lathe works in a csg. Surface coordinates: u around (0 to 1 over the sweep), v
along the profile by length (0 to 1, the caps included).

**Extrusions and sweeps.** An `extrusion` is built like text's letters: its outlines' curves are
flattened to within 0.1% of their size, holes found from how they nest, the face (at z = 0, facing
−z) cut into triangles, and the shape pushed `depth` back along +z into a closed mesh, the sides
smooth around curves and sharp at corners of more than 35°; then `rotation` turns it and `position`
moves it. `bevel` and `backBevel` cut the front and back edges at 45°. `taper` scales the shape
toward the back and `twist` turns it (counterclockwise seen from the front), both about the
outline's origin; the sides of a twisted extrusion are cut into slices along the depth, one each 2°
of twist. Bevels can't be used with a taper or a twist. Surface coordinates: x and y of the
outline. A `sweep` carries a circle of `radius`, or any `outline`, along its `path`, turning it as
little as it can (a rotation-minimizing frame, by double reflection) so the tube doesn't twist; a
closed path's frame is evened out so the tube meets itself. It is built into a smooth mesh: `steps`
pieces along each span of a smooth path (or one a segment of a straight one) and `segments` around
a round tube, and an open path's ends are closed with flat caps unless `capped` is false. Surface
coordinates: u around the cross-section, v along the path by length. Both work in a csg and take
any material. `tools/make_shape_scenes.py` writes `lathe_test.ray` and `extrusion_test.ray`.

**Bezier patches and B-spline surfaces.** A `bezier` is a surface of Bezier patches, each with a
degree in u and v (bicubic, 3 by 3, is the usual) and (u degree + 1) × (v degree + 1) control
points, row by row with u varying fastest. A point with a weight (`[x, y, z, w]`, more than 0)
makes its patch rational. Patches can be listed with their own `points`, or as `indices` into the
shape's own `points`, as the Utah teapot is usually written; or read from a `file` in the `.bpt`
format: the number of patches, then for each its two degrees and its points, `x y z` a line.
`models/teapot.bpt` holds Martin Newell's 1975 teapot (32 bicubic patches, z up, the spout along
+x), so `"rotation": [-90, 0, 0]` stands it on y like the mesh teapot. A patch file goes into a
saved scene's archive and to networked engines inside the scene (as `data`), like a model file.
The points are in the shape's own frame: scaled by `scale`, turned by `rotation` and moved to
`center`.

The patches are traced exactly, not cut into triangles: each is split by de Casteljau's algorithm
until every piece is within 0.02% of the shape's size of flat; a tree of the pieces' boxes finds
the ones a ray passes near; a start from the triangles of each piece's corners and Newton's method
find where the ray meets the surface itself. A hit anywhere on a patch counts, not only inside the
piece that found it, so nothing slips through the seams between pieces or patches. The normal is
the cross product of the surface's slopes; where it vanishes (an edge that shrinks to a point, as
at the teapot's knob and base) it is taken a hair inside the patch. Patches are open and shaded on
the side a ray sees, unless `"closed": true` says they enclose a volume with their normals outward,
so a csg can use them. Surface coordinates are each patch's own (u, v), from 0 to 1, or across its
`uv` rectangle (`[u0, v0, u1, v1]`), so a texture can run across several patches.

A `bspline` is a B-spline surface, or with weights a NURBS surface (circles, spheres and tori come
out exact): `points` is its control net, rows along v, each row along u, with a `degree` each way
and a knot vector each way (`knotsU`, `knotsV`; left out, they are clamped, so the surface reaches
its corner points, and evenly spaced). It is traced as Bezier patches: knots are inserted (Boehm's
algorithm, with weights carried along) until every knot in its range repeats as often as the
degree, which cuts it into patches that are the same surface exactly. Its surface coordinates run
from 0 to 1 across the whole surface. Bump maps, textures and maps work on both.

**Implicit surfaces, blobs and fractals.** These three are defined by a number at every point:
negative inside, zero on the surface. Each is worked out at size 1 in its own frame, scaled by
`scale` (default `1`, a number), turned by `rotation` and moved to `center`. Surface coordinates
are projected onto a sphere around the center: u around +y, v from the bottom (0) to the top (1).
Bump maps, textures and maps work on all three.

- An **implicit** surface is where its `formula` is zero, inside where it is negative, within a box
  of half-sizes `bounds` (a number, or `[x, y, z]`) around the center. The formula is in x, y and z:
  numbers (`2`, `0.5`, `1e-3`); the constants `pi` and `e`; `parameters`, an object of names and
  numbers (`{ "k": 5 }`) the formula can use; `+ - * /`; `^` for a power, grouped from the right
  (`2^3^2` is 2⁹), with unary minus below it (`-x^2` is −(x²)) and a negative exponent allowed
  (`2^-1`); parentheses; and the functions `sin`, `cos`, `tan`, `asin`, `acos`, `atan`,
  `atan2(y, x)`, `sqrt`, `abs`, `exp`, `log`, `pow(a, b)`, `min(a, b)`, `max(a, b)`, `floor`,
  `clamp(x, low, high)`, `mix(a, b, t)` and `smoothstep(edge0, edge1, x)`. A mistake is reported
  with its column (`formula: column 5: the formula ends too soon`). A ray is followed through the
  box in steps of the box's diagonal over `steps` (default `256`), and the surface is found where
  the formula changes sign between two steps, then pinned down by bisection to rounding. With
  `lipschitz`, the most the formula can change per unit of distance (1 for a true distance), a step
  is |f| / lipschitz where that is longer, which is faster and as safe. A feature thinner than a
  step can be stepped over, and a place where the formula touches zero without changing sign is
  missed: raise `steps` for fine detail. The cost is up to `steps` evaluations a ray through the
  box. The normal is the formula's gradient, worked out exactly (the derivatives are carried through
  every operation). It is a solid, so a csg can use it (`implicit_test.ray` cuts a gyroid's
  labyrinth down to a ball).
- A **blob** is metaballs: `balls`, each `{ "center": [x, y, z], "radius": r, "strength": s }`
  (`strength` default `1`; negative dents the others), adds s (1 − d²/r²)³ within its radius (d the
  distance from its center) and nothing beyond, and the surface is where the sum is `threshold`
  (default `0.5`, more than 0). A lone ball is a sphere of radius r √(1 − (threshold / s)^(1/3)).
  Ball centers are in the blob's own frame. Along a ray each ball's field is a polynomial of degree
  6, so between the places the ray enters and leaves balls the sum is one polynomial and its roots
  are found exactly. At least one ball must have positive strength. It is a solid.
- A **fractal** is traced by sphere tracing a distance estimate: the ray steps as far as the
  estimate says is safe (times `stepFactor`, default `1`, at most 1) until it is within `epsilon`
  (default `0.0001`, at size 1) of the surface, for at most `maxSteps` (default `256`) steps. A
  smaller epsilon shows finer detail and takes more steps. `kind` is `"mandelbulb"` (White and
  Nylander's 3D Mandelbrot set, +y the pole: `power` default `8`, from 2 to 32; `iterations`
  default `10`; `bailout` default `2`; within a box of half-size 1.25 for power 6 and up),
  `"menger"` (the Menger sponge, a cube of half-size 1 with its middle thirds drilled out
  `iterations` times, default `3`; with 0 it is the cube) or `"sierpinski"` (the Sierpinski
  tetrahedron, corners at (1, 1, 1), (−1, −1, 1), (−1, 1, −1) and (1, −1, −1), folded
  `iterations` times, default `6`; `"rotation": [-45, 0, 35.264]` stands it on a face).
  `iterations` is from 0 to 64. Only a mandelbulb has a `power` and a `bailout`. The normal is the
  estimate's gradient, by differences. Sphere tracing can't start inside, so a fractal isn't a
  solid a csg can use.

**Constructive solid geometry.** A `csg` is a solid made from others: a `"union"` is inside any of
them, an `"intersection"` inside all of them, and a `"difference"` inside the first and none of the
rest. Its `objects` are written as in the scene's own list, and can be any closed shapes: spheres, tori, superquadrics, rounded boxes, implicit surfaces, blobs,
boxes, capped cylinders, cones, paraboloids and hyperboloids, text, closed patches, meshes (which must be closed; a mesh with holes gives wrong
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

**Model files.** A mesh can take its triangles from a `file`, relative to the scene file as
texture images are, and goes into a saved scene's archive (and, as `data`, into the copy of the
scene each networked engine gets) the same way:

- **STL**, binary or ASCII. A binary file is told by its size (80 bytes of header, a count, and 50
  bytes a triangle), since many begin `solid` as ASCII ones do. Corners at exactly the same place
  are joined, so `"smooth": true` can blend across faces; the facet normals in the file are ignored,
  and each face's corners run counterclockwise seen from outside, as the standard says.
- **Colored STL**, in either informal convention: Materialise Magics (`COLOR=` and the model's
  red, green, blue and alpha bytes in the header; a face whose attribute has bit 15 clear has its
  own color, red in bits 0–4, green 5–9, blue 10–14; with it set the model's) or VisCAM and
  SolidView (no `COLOR=`; a face whose attribute has bit 15 set has a color, blue in bits 0–4, green
  5–9, red 10–14). The colors show through the `faceColors` texture (below), on whichever materials
  should show them.
- **OBJ**: `v`, `vt`, `vn` and `f` lines. A face can have any number of corners (it is split into
  a fan of triangles), each `v`, `v/vt`, `v//vn` or `v/vt/vn`, counted from 1 or, negative, back
  from the last one read. Texture coordinates are kept only if every corner has them, normals
  likewise, and an OBJ's normals make the mesh smooth. Objects, groups, smoothing groups and
  materials (`o`, `g`, `s`, `usemtl`, `mtllib`) are ignored: the mesh's own `materials` cover all of
  it.
- **glTF 2.0** (added in 2026-alpha-0.20.1): `.gltf` (JSON, its buffers in `data:` URIs or in
  files beside it) or `.glb` (the binary container, told by its `glTF` magic whatever its name).
  Every mesh of the default scene is read, each node's `matrix` or `translation`, `rotation` and
  `scale` flattened in; triangles, strips and fans (points and lines are skipped); accessors of
  every component type, with `byteStride`, normalized integers and sparse substitution; `POSITION`,
  `NORMAL` (kept if every primitive has them, and then the mesh is smooth) and `TEXCOORD_0` (likewise).
  Each primitive's material's `pbrMetallicRoughness` `baseColorFactor` becomes its faces' color,
  times its `baseColorTexture` at the middle of each face when that image is a PNG (in the file,
  beside it, or in a `data:` URI); a JPEG or WebP image is skipped and the factor alone used (a
  texture's plain `source`, often a PNG fallback beside a WebP, is used when there is one). The
  colors show through the `faceColors` texture, as a colored STL's do. Metallic and roughness,
  normal maps, cameras, lights, skins, morph targets and animation are ignored. glTF is right-handed
  and Ray's scenes are left-handed, so z is negated and each face turned back round: the model isn't
  mirrored. Draco-compressed primitives (`KHR_draco_mesh_compression`, added in
  2026-alpha-0.20.2) are decoded by Ray: Draco bitstream 2.2, sequential or EdgeBreaker connectivity
  (standard or valence traversal), and every prediction scheme Draco writes (difference,
  parallelogram, multi-parallelogram, portable texture coordinates, geometric normals); each corner
  of the decoded mesh becomes a vertex, joined where all its values match. meshopt-compressed files
  are refused with the extension's name. A `.gltf`'s buffers and PNG images
  go into a saved scene's archive beside it, under the names it gives them, and into a networked
  engine's copy of the scene (as `files`, an object of base64 data by URI); a URI that leaves the
  model's folder is refused.

- **Blender** (added in 2026-alpha-0.20.2): `.blend` files saved by Blender 2.80 to 4.x (Blender
  5 changed how meshes are stored and is refused with a reason), plain, gzip-compressed (Blender 2.x)
  or Zstandard-compressed (3.0 and later). Every mesh object in the file is read (or only the one a
  mesh's `node` names, without Blender's `OB` prefix), each at its world placement: the matrix the
  file stores, or, for files from 4.2 on, which store none, the one built from its location,
  rotation (any Euler order, quaternion or axis-angle), scale, delta location and scale, and parent
  (an object parent, with its parent inverse). Both of Blender's mesh layouts are read: up to 3.x,
  MVert, MPoly and MLoop arrays and MLoopUV layers; from 3.5 on, the `position` attribute, face
  offsets, `.corner_vert`, float2 UV maps and the `material_index` face attribute. Faces of any size
  are cut into fans of triangles. The active UV map gives surface coordinates, and each material
  slot's viewport color (`Material` `r`, `g`, `b`; not its node tree) colors its faces through
  `faceColors`, an object's own material taking a slot where the object says so. Modifiers,
  constraints, drivers, animation, bone parents, curves, text, metaballs, instancing and linked
  library data are not evaluated or followed; Blender is z up and right-handed, so (x, y, z) becomes
  (x, z, y) and each face is turned back round.

A file is named `.stl`, `.obj`, `.gltf`, `.glb` or `.blend`; another name is told apart by its
contents. The model is placed
by these fields, in this order:

| Field | Default | |
|-------|---------|---|
| `scale` | `1` | A number, or `[x, y, z]`; none can be 0. A negative one mirrors the model (its faces still face out) |
| `fitSize` | | Instead of `scale`: scales the model evenly so that, turned, it is this wide at its widest |
| `rotation` | `[0, 0, 0]` | Degrees about x, then y, then z, as a box turns. Most printable models stand on z: `[-90, 0, 0]` stands them on y |
| `align` | `"none"` | `"center"` centers the turned model's box on the origin; `"bottom"` centers it in x and z with its lowest point at y = 0 |
| `translation` | `[0, 0, 0]` | Moves it, last |
| `node` | | glTF: read just the node with this `name` and its children, in that node's own frame (its own and its parents' placement left out), as for one piece of a chess set. Blender: read just the mesh object with this name, at its world placement |
| `gltfMaterial` | | glTF only: read just the primitives whose material has this `name`, so one part of a model (a visor) can wear different materials from the rest |

`smooth`, `doubleSided`, `uvMap`, `materials` and `bump` work as for any mesh. Textures and bump
maps on a model are looked up in the file's own coordinates, before its placement, so they move,
turn and grow with it: a solid texture's `scale` is in the file's units (millimeters, for a
printable model), and the `layers` texture follows the print axis however the model is placed. A
large file is read once however many copies of the scene a render makes, but each copy (one a
thread) builds its own faces, about 460 bytes a triangle: 3DBenchy takes about 250 MB a thread.

```json
{ "type": "mesh", "file": "models/3DBenchy.stl", "fitSize": 2, "rotation": [-90, 0, 0], "align": "bottom",
  "translation": [-4, 0, -7], "materials": [ { "type": "diffuse", "color": [1, 1, 1], "texture": { "type": "layers" } } ] }
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
| `metal` | A conductor (added in 2026-alpha-0.20.1): a mirror whose reflectance in red, green and blue comes from the metal's complex index of refraction n + ik by the exact Fresnel equations for a conductor, so gold is yellow face on and whitens toward grazing. `metal` names a preset (`gold`, `silver`, `copper`, `aluminum`, `iron`, `chromium`, `titanium`, `platinum`: representative optical constants near 650, 550 and 450 nm), or `n` and `k` (`[red, green, blue]`) give the index (and override a preset's); `roughness` (default `0`, up to `1`) blurs the reflection as a rough mirror's does; `tint` (default white; a color or a map) multiplies it. A gold visor is `metal` at most of the influence beside a `transparent` material for what gets through |
| `regolith` | Lunar soil after Hapke: the Lommel-Seeliger law (light scattered back toward where it came from, with no limb darkening) times an opposition surge that brightens it where the viewer looks along the light. `surge` (default `1`, Hapke's B0) is the surge's strength and `width` (default `0.06`, his h) its angular width; seen along the light at normal incidence it is exactly `color` (default `[0.12, 0.12, 0.12]`, about the Moon's albedo) |
| `specular` | A Cook–Torrance highlight. `power` (default `1`) is the surface's Beckmann roughness: smaller is a smaller, sharper highlight (0.1–0.3 is glossy; 1 is very rough). `indexOfRefraction` (default `1.5`) sets the Fresnel reflection, stronger at grazing angles; `materialHighlight` (default `false`) |
| `reflective` | A mirror reflection, tinted by `color` with `filterReflection` (default `false`); `diffuseReflectivity` (default `0`). `roughness` (default `0`, a perfect mirror; up to `1`) blurs the reflection, as of brushed or satin metal: each ray reflects about a microfacet normal drawn from a GGX distribution of that roughness (0.05 is satin, 0.3 very rough) |
| `transparent` | Light passing through, bent by Snell's law at `indexOfRefraction` (default `1`, which doesn't bend; glass is about 1.5), and reflected inside past the critical angle; tinted by `color` at each surface it crosses. `attenuationColor` (default `[1, 1, 1]`, each 0 to 1) and `attenuationDistance` (default `0`, off) absorb light by how far it goes inside: after `attenuationDistance` it is `attenuationColor`, after twice that the color squared (Beer–Lambert). `roughness` (default `0`, clear; up to `1`) frosts the glass: each ray bends through a microfacet normal drawn from a GGX distribution of that roughness, so what is behind is blurred (0.05 lightly frosted, 0.3 heavily). `abbe` (default `0`, no dispersion; otherwise at least 5) is the glass's Abbe number: a ray splits into red, green and blue at the glass, each bent by its own index from a Cauchy curve through `indexOfRefraction` (green, the d line) and the Abbe number, as a real lens's glass is (crown glass is about 60, flint 30 to 40, very dense flint 20) |

Absorption is measured from where a ray goes into an object through its transparent material to
where it next meets that object, so the object should be closed (a sphere, a capped cylinder or
cone, a box, or a closed mesh). A ray turned back inside past the critical angle keeps absorbing.

**Textures inside glass.** When `attenuationColor` or `attenuationDistance` is a texture (see
**Texture maps** below), the absorption varies inside the object: it is added up along the ray's
path, `attenuationStep` (default `0.05` scene units) at a time, each step taking away −ln(color) ×
step / distance with the texture read at a point within the step (shifted by the ray's sample, so a
pixel's rays read between each other's). A solid texture then colors the glass from within:
swirls of art glass (`marble`), smoky quartz (`noise`), dark flecks in amber (`cells`), or veins
floating in a clear block. A texture that is the same everywhere absorbs exactly as the constant
does. Each step costs a texture lookup, so smaller steps are finer and slower; a step about a
tenth of the texture's features is plenty. Spectral dispersion uses the same absorption for
every wavelength. `volume_test.ray` shows all four.

**Texture maps.** Every number and color of a material can be a texture instead of a constant,
so how see-through, rough, refractive or reflective a surface is can change from place to place:
`color` (of any material), `influence`, a specular's `power` and `indexOfRefraction`, a
reflective's `roughness` and `diffuseReflectivity`, and a transparent's `indexOfRefraction`,
`roughness`, `attenuationColor` and `attenuationDistance`; and a bump map's `depth` and
`strength`. A setting is written as before (a number, or a `[red, green, blue]` array), or as a
texture:

```json
"roughness": { "type": "cells", "mode": "flecks", "colors": [[0, 0, 0], [1, 1, 1]] }
"roughness": { "texture": { "preset": "travertine" }, "scale": 0.4, "offset": 0.05, "channel": "red" }
```

The first is any texture or texture preset, read as it is. The second wraps one: a number is
`offset` (default `0`) + `scale` (default `1`) × the texture's `channel`, `"average"` of red, green
and blue (the default), `"luminance"`, `"red"`, `"green"` or `"blue"`; a color is the texture's
color × `scale` + `offset`. A texture is read at the point being shaded, as a material's texture
is (solid textures need nothing; images need the object's surface coordinates). Values it gives
are kept within the setting's range (roughness 0 to 1, an index from 0.001 to 50); a constant is
checked when it is read, as before. A material's `texture` still takes the place of its `color`.
Constants are saved exactly as before, so files without maps don't change. `maps_test.ray` shows
frosted and polished patches, a varying index, a lace of glass and paint by `influence`, bumps of
varying depth and a light colored by a 3D texture.
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

With the camera's `"dispersion": "spectral"` a split makes four single wavelengths instead, one
from each quarter of 380 to 720 nm, all at the same place in their quarters. Where that place is
comes from which of its pixel's samples the ray is: the samples take one each of as many equal
strata, in an order shuffled by a hash of the pixel, so a pixel's wavelengths spread evenly over
the range and have nothing to do with where in the pixel each sample falls. (Samplers that make a
fixed number of rays a pixel say which each is; with `corners` and `none`, the place is a hash of
the sample point.) Each is bent by the Cauchy index at its own
wavelength, and what it sees is weighed by how that wavelength looks (the CIE 1931 color matching
functions, as linear sRGB, scaled so an even spread of wavelengths adds up to white). Over a
pixel the colors smear into a continuous rainbow, with the yellows and cyans three bars can't
show. A chromatic lens is traced the same way (four wavelengths a sample). It costs a third more
rays than red, green and blue, and the smear is a little grainier, so it wants a sampler with many rays a
pixel; the purest spectral colors are outside sRGB, so they come out a little dimmer. Scenes
without dispersive glass or a chromatic lens render exactly the same either way.

**Textures.** Any material can have a `texture` that gives its color at each point instead of its
`color`. A texture is an object with a `type`:

| `type` | Fields |
|--------|--------|
| `checker` | `colors` (two colors, default black and white), `size` (default `1`), `solid` (default `false`: squares in surface coordinates; `true`: cubes in the scene, which needs no surface coordinates) |
| `marble` | Bands across `axis` (default `[1, 0, 0]`), one each `scale`, bent by turbulence into veins: the ramp's first color is the veins, its last the stone. Default `turbulence` `2.5`; default colors white marble with gray veins |
| `wood` | Rings about the line through `origin` along `axis` (default `[0, 1, 0]`), one each `scale`, wobbled by noise; each ring runs through the ramp, its last color the dark edge. Default `turbulence` `0.3`; default colors pine |
| `noise` | The noise itself, for stone, granite or clouds: with `turbulence` `0` (the default), smooth layered noise through the ramp; with more, `turbulence` times the turbulence function, which has sharp creases. Default colors black to white |
| `layers` | The layer lines of a 3D print: rounded ridges, one each `scale` (default `0.1`) along `axis` (default `[0, 0, 1]`, the print direction of a model file), 0 at the seams and rising along a parabola to 1 in the middle of each layer, through the ramp. As a color it darkens the seams (default colors light gray to white); as a bump map's texture, with colors black to white, it gives each layer its rounded edge. `turbulence` (default `0`) wobbles the layers slowly, its noise about twenty layers across |
| `cells` | Worley's cellular noise: space split into cells around points scattered one a unit cube (one each `scale`). `mode`: `"flecks"` (the default: one value all over each cell, so each cell is a chip of a ramp color: granite, terrazzo), `"distance"` (the distance to the nearest point, about 0 to 1: spots) or `"edges"` (the gap to the second nearest, 0 on the cells' boundaries: cracks, scales). `turbulence` (default `0`) moves the point by noise first, so the cells' sides wander. Default colors black to white |
| `caustics` | The web of bright lines light makes under water: the edges of Worley cells, bent by `turbulence` (default `0.6`), 1 on a line and falling to 0 a `width` (default `0.12`, a fraction of a cell) away, in `layers` (1 to 3, default 2) turned and scaled against each other. Meant for a light's `filter` (below), where it adds light along the lines: the default ramp runs from a dim blue-gray to a white brighter than 1 |
| `faceColors` | The colors a colored model file gives its faces; `default` (default `[0.8, 0.8, 0.8]`) where a face has none or the surface isn't from such a file |
| `image` | `file` (required: a PNG, a PPM, P6 or P3, or a Ray raw image, relative to the scene file), `scale` (`[u, v]` repeats, default `[1, 1]`), `offset` (`[u, v]`, default `[0, 0]`), `repeat` (default `true`; `false` stretches the edge pixels), `srgb` (default `false`: a byte is the color /255, matching the default color model; `true` decodes the sRGB curve) |

`marble`, `wood`, `noise`, `layers`, `cells` and `caustics` are solid: they fill space, so an object looks carved from a block and
needs no surface coordinates. They share these fields: `colors` (a ramp of two or more colors,
evenly spaced and blended between), `scale` (default `1`: the size of the pattern in scene units),
`origin` (default `[0, 0, 0]`: where the pattern is centered), `stretch` (default `[1, 1, 1]`:
the scale along x, y and z on top of `scale`, so `[40, 1, 1]` draws features forty times longer
along x, as brushed metal has; written only when it isn't the default), `turbulence`, `octaves` (default
`6`, from 1 to 12: layers of noise, each twice as fine and half as strong), and `seed` (default
`0`: each seed is a different block, and the same seed always the same one). The noise is Perlin's
gradient noise. The pattern stays put in the scene, so moving an object moves it through the block,
except on a mesh loaded from a file, whose textures are looked up in the file's own coordinates.

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

**Presets: a library of natural materials.** Instead of building a stone or a wood from a ramp of
colors and the right noise, a scene can name one from Ray's library and change what it likes.
The library is part of Ray, so a scene that names a preset needs no other file and networked
engines have it too. A preset can be named in two places:

- **As a texture**, anywhere a texture goes (a material's, a bump map's, a light's `filter`):
  `{ "preset": "carrara-marble" }` is the preset's texture, and any other field beside it
  replaces the preset's own: `{ "preset": "oak", "scale": 0.03, "axis": [1, 0, 0] }` is oak with
  finer rings, its grain along x.
- **As a whole finish**, as an entry of an object's `materials`: `{ "preset": "carrara-marble" }`
  stands for the preset's materials, its texture with a finish that suits it (polished stones and
  gems: ambient, diffuse, a sharp highlight and a hint of blurred reflection; honed stone a broad
  sheen; rough stone, sand, clay and rust matte; woods a soft highlight; metals a blurred
  reflection tinted by the texture). Beside `"preset"`, `"texture"` is laid over the texture of
  every material that has one, and a material type (`"ambient"`, `"diffuse"`, `"specular"`,
  `"reflective"`, `"transparent"`) is laid over the preset's material of that type, added if it has
  none, or taken out if it is `null`:
  `{ "preset": "black-marquina", "texture": { "seed": 5 }, "specular": { "power": 0.3 }, "reflective": null }`.
  A preset entry can sit among other materials.

Laying over merges objects field by field and replaces anything else. A scene saved again keeps
the preset's name and only the fields given beside it, as long as they still give exactly the
texture or materials they gave when the scene was read; changed some other way since (in an
editor), the full texture or materials are saved instead, so nothing is lost.

| Group | Presets |
|-------|---------|
| Stone | `carrara-marble` (white, soft gray veins), `black-marquina` (black, sharp white veins), `verde-marble` (dark green serpentine), `rosso-marble` (red, cream veins), `travertine` (beige bands), `granite` (gray and pink flecks), `black-granite`, `slate` (blue-gray, in fine layers), `sandstone` (wavy red-gold strata), `basalt`, `sand`, `concrete`, `terrazzo` (cream with colored chips) |
| Wood (rings about +y; give `axis` to turn the grain) | `oak`, `pine`, `walnut`, `cherry`, `mahogany`, `ebony`, `birch` |
| Gem | `jade`, `amber`, `obsidian`, `malachite` (concentric green bands), `lapis-lazuli` (deep blue, gold flecks) |
| Metal (grain along x) | `brushed-steel`, `brushed-gold`, `brushed-copper`, `rust` |
| Other | `terracotta`; `caustics` (a texture only, for a light's `filter`) |

The textures follow the classic procedural recipes: marble as bands bent by turbulence and wood as
rings wobbled by noise (Perlin, "An Image Synthesizer", SIGGRAPH 1985; "Improving Noise", 2002),
stones and gems as noise or turbulence through a ramp of colors, and granite and terrazzo as
cellular flecks (Worley, "A Cellular Texture Basis Function", SIGGRAPH 1996), as gathered in
Ebert, Musgrave, Peachey, Perlin and Worley, *Texturing and Modeling: A Procedural Approach*.
The colors are Ray's own, matched by eye. Presets are kept in `core/src/Presets.cpp`.

**Lights.**

| `type` | Fields |
|--------|--------|
| `point` | `position` (required), `color` (required). A point: sharp shadows |
| `sphereLight` | `position` (required): its center; `color` (required); `radius` (default `1`); `samples` (default `1`, up to 256). A glowing ball: soft shadows |
| `rectangleLight` | `position` (required): its center; `color` (required); `edge1` and `edge2` (required): its sides, as vectors; `samples` (default `1`, up to 256); `angle` (default `0`: no limit) and `falloff` (default `0`): with an angle, it shines in a beam that many degrees from its facing, its edge fading over the last `falloff` degrees, like a softbox with a grid, and takes a `profile` (below). A glowing panel, shining only to the side `edge1` × `edge2` points to: soft shadows |
| `spot` | `position` (required); `direction` (required): which way it points; `color` (required); `angle` (default `30`): degrees from its axis to the edge of its cone, more than 0 and less than 90; `falloff` (default `5`): degrees inside the edge over which it fades (a smoothstep, or as its `profile` says), from 0 (a hard edge) to the angle; `radius` (default `0`) and `samples` (default `1`, up to 256). A light in a cone; with a radius it is a disc (seen from each point as a sphere light is), with soft shadows |
| `directional` | `direction` (required): the way the light travels; `color` (required); `angle` (default `0`): its angular diameter in degrees, less than 90 (the sun is about 0.53); `samples` (default `1`, up to 256). Sunlight: the same light from the same direction everywhere, its shadows the size of what casts them; with an angle, soft shadows that blur the farther they fall. It has no position |

An area light (`sphereLight`, `rectangleLight`, and a `spot` with a radius or a `directional` with
an angle) shades each point from `samples` of its points, each with its own shadow ray and an even
share of its `color`; a sphere light's and a spot's points are on the disc it shows the point, and
a sun's on its disc in the sky. Like a point light, every light gives the same light at every
distance (unless it has an `attenuation`) and the camera doesn't see it. A shadow ray toward the
sun (or the sky) goes 10⁶ scene units. Glass casts shadows as if it were opaque, as before.

Every light can also take these, all off unless given, so a light without them shines exactly as
before:

| Field | Default | Meaning |
|-------|---------|---------|
| `attenuation` | `"none"` | How the light weakens with distance `d`: `"none"` (the same everywhere), `"inverse"` (by `distance` / d) or `"inverseSquare"` (by (`distance` / d)², as real light does). Not for `directional` |
| `distance` | `1` | Where an attenuated light is its full `color` |
| `tint` | `[1, 1, 1]` | A color the light is multiplied by at the point it lights: a color, or a texture map (see **Texture maps**) read at that point in the scene, so what light arrives depends on where it lands. Use a solid texture (`marble`, `noise`, `cells`, `caustics`): the lit point has no surface coordinates here |
| `intensity` | `1` | A number the light is multiplied by there (0 or more), or a texture map read the same way: a `noise` intensity on sunlight gives drifting cloud shadows |
| `filter` | none | An image or pattern the light shines through, like a slide in a projector or a gobo in a stage light: `{ "texture": ..., "size": 1 }`, any texture or texture preset, which colors the light it sends each way. A `spot` projects it across its cone: an image's (u, v) run from 0 to 1 over the square around the cone, and a solid texture reads the same place with the cone's edge `size` from its middle. A `directional` light projects it straight along its direction: a point is moved onto the plane across the light, an image spans `size` scene units, and a solid texture reads the point in units of `size`, so the pattern is the same all along the light. A `rectangleLight` projects it along its facing, the image over the rectangle. A `point` or `sphereLight` wraps it all around: an image by longitude and latitude, a solid texture on a sphere of radius `size`. With the `caustics` preset under haze, sunlight gives the rippling light seen under water (`underwater_test.ray`) |

A `spot`, and a `rectangleLight` with an `angle`, shape their beam with a `profile`. Each is the
fading edge (over the last `falloff` degrees) times a shape across the cone, by the angle θ from
the axis:

| `profile` | Shape |
|-----------|-------|
| `"smoothstep"` | The default: even all across, the edge fading along a smoothstep (as before) |
| `"linear"` | Even all across, the edge fading in a straight line |
| `"cosine"` | cos^`exponent`(θ) (`exponent` default `8`): a soft hot center, like a bare bulb in a reflector |
| `"gaussian"` | exp(−½ (θ / `spread`)²) (`spread` default a third of the angle): a smooth beam with no visible edge |
| `"fresnel"` | Full out to `hotspot` degrees (default a third of the angle), falling smoothly over as wide again to `rim` (default `0.35`), which holds out to the edge: the bright center and wide, even spill of a stage Fresnel |
| `"table"` | `table`: brightnesses at even steps from the axis (first) to the edge (last), blended between, like a photometric (IES) table: a real fixture's measured beam |

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
  (2026-alpha-0.10), and the camera's `dispersion` (2026-alpha-0.11.1), and the settings' `indirect` and the scene's `haze` (2026-alpha-0.13), and presets
  (`"preset"` in a texture or a materials entry), the `cells` and `caustics` textures, a solid
  texture's `stretch`, and lights' `attenuation`, `filter` and beam `profile` (2026-alpha-0.14),
  and texture maps for material, bump and light settings, and a transparent material's
  `attenuationStep` (2026-alpha-0.15), and the `torus`, `disc`, `annulus`, `paraboloid`, `hyperboloid`,
  `superquadric`, `roundedBox` and `text` objects (2026-alpha-0.16), and the `bezier` and `bspline`
  objects (2026-alpha-0.17), and glTF model files with a mesh's `node`, `gltfMaterial` and `files`,
  the `metal` material, and the adaptive sampler's `confidence` and `channels` (2026-alpha-0.20.1).

`renders/` holds output files from 1997. `scan_test.raw` there is a partial trace from that time
(a few scan lines over black), not a reference image.
