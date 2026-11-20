# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions up to
0.2.0 followed [Semantic Versioning](https://semver.org/spec/v2.0.0.html); from 2026 they are named
for the year, the stage and a number (`2026-alpha-0.3`).

Every change that alters rendered output shows its before and after images here.

## [Unreleased]

## [2026-alpha-0.7] - 2026-11-20

Softness: frosted glass and brushed mirrors, and area lights that cast soft shadows, all picking
their directions and points from where each ray falls in its pixel, so a stratified sampler
smooths them and every render is the same each time. Scenes without the new fields render byte
for byte as before.

### Added

- **Rough glass and mirrors.** `transparent` and `reflective` take a `roughness` (default `0`, up
  to `1`). Each ray bends or reflects about a microfacet normal drawn from a GGX
  (Trowbridge–Reitz) distribution of that roughness about the surface's normal, so frosted glass
  blurs what is behind it and a brushed or satin mirror blurs its reflection; a microfacet that
  would send the ray the wrong way through the surface is passed over for the smooth direction.
  Smooth materials don't write a `roughness`, so existing scenes save as before.
- **Area lights: `sphereLight` and `rectangleLight`.** A glowing ball (`radius`) or panel (sides
  `edge1` and `edge2`, shining only to the side `edge1` × `edge2` points to), each with `samples`
  (default `1`, up to 256). Each point is shaded from that many points of the light, each with its
  own shadow ray and an even share of its color, so shadows are dark where none of the light can be
  seen and fade through a penumbra where some can. Like point lights they don't fall off with
  distance and the camera doesn't see them; point lights are unchanged.
- **Where the samples come from.** Each ray carries a sample point (where its primary ray fell in
  its pixel) and a key (from its pixel and the bounces since). A rough surface or an area light uses
  that point, shifted, turned and mirrored by a hash of the key, its depth and what it's for, so a
  sampler that spreads its rays evenly over a pixel (`stratified`, `adaptive`) spreads them evenly
  over the blur and the light. A ray made at a surface picks again from a point hashed from its
  parent's: reusing the same point there tied each bounce to the one before in a way fixed for the
  pixel, and frosted glass came out with a speckle that more rays never smoothed. Nothing is random,
  so networked engines match threads byte for byte; nothing new crosses the wire.
- **`scenes/soft_test.ray`**: three glass panes before a striped poster (clear, roughness 0.08 and
  0.3), a mirror ball and a brushed gold one, and soft shadows from a rectangle light and a sphere
  light, with a 6×6 stratified sampler. New tests `render_soft` (with a reference render and a
  README preview), `roundtrip_soft_test` and `network_soft`. `format_test` checks that the new
  fields read, save and read back, that bad values are reported, that GGX microfacets tilt as the
  distribution says (half within atan of the roughness), that a sphere light's points lie on the
  disc it shows, that a rectangle light shines only to its front, that hiding half of one gives half
  its light, and that rough glass is the same every time for the same ray.

  ![Before and after: the soft test with smooth glass and point lights (left) and with frosted glass, a brushed ball and area lights (right); below, the torture test before and after](docs/images/changelog/soft-lights.png)

  Above, the new test scene drawn with what the renderer had before (smooth surfaces, point lights
  at the area lights' centers) and as it is now: the middle and right panes blur the stripes behind
  them, the gold ball's reflection is brushed, and the shadows of the post and the red ball are soft
  at their edges. Below, the torture test: the glass ball is frosted, the gold tube brushed, and the
  shadows soften under the new sphere and rectangle lights.

### Changed

- **The torture test uses the new features:** the glass sphere is frosted (roughness 0.12), the
  gold tube brushed (0.15), the key light a sphere light of radius 3 and the overhead fill a 3 × 3
  rectangle light. Its reference render, README preview and full-size image are regenerated; its
  description in `scenes/README.md`, which still had the copper teapot and black frame, is brought
  up to date.
- **`CLAUDE.md`: every new feature goes into the torture test** in the same change, with its
  reference render, README preview and full-size image regenerated; and every release updates the
  release gallery.

## [2026-alpha-0.6] - 2026-11-20

Solid noise textures (marble, wood, and noise for granite and stone) and glass that absorbs light
by how far it travels inside, shown in a new test scene and worked into the torture test.

### Added

- **Solid noise textures: `marble`, `wood` and `noise`.** They fill space rather than lie on a
  surface, so an object looks carved from a block of the material and needs no surface
  coordinates. Each works out a value from Perlin gradient noise (his 2002 form, with a shuffle
  picked by `seed`) and looks it up in a ramp of `colors`. `marble` draws thin veins where bands
  across `axis` meet, bent by `turbulence`. `wood` draws rings about an axis, wobbled by noise.
  `noise` gives smooth layered noise, or sharp-creased turbulence for granite and stone. All take
  `scale`, `origin`, `turbulence`, `octaves` and `seed`; see `scenes/README.md`.
- **Glass that absorbs by the distance light travels inside it.** A `transparent` material's new
  `attenuationColor` and `attenuationDistance` give the color light is left after going that far
  through the object. Twice as far gives that color squared (Beer–Lambert). Thick glass is deeper
  in color than thin, and a glass ball is darker through its middle than at its rim. Each ray
  carries the transparent object it is inside (`CRay::GetInside`), so the surface where it comes
  out knows how far it went. A ray turned back inside past the critical angle keeps absorbing.
  Without the new fields nothing changes, and every existing render is byte for byte the same.

  ![Before and after: green glass slabs 0.15, 0.5 and 1.2 thick and an amber ball, tinted flat by color (left) and absorbing by distance (right)](docs/images/changelog/absorption.png)

  On the left, the old way: a flat tint at each surface, so all three slabs are the same green. On
  the right, absorption: the thin slab is pale and the thick one dark. The amber ball is deepest
  through its middle and lightens toward its rim.
- **The torture test uses them too.** The teapot is polished white marble instead of copper, the
  mirror's frame is wood (and a little wider, 16.2 by 5.6, so the grain shows round the mirror),
  and the glass cube is clear glass that turns green by the distance light goes through it. The
  `torture_test` reference render, its README preview and the full-size README image are
  regenerated.

  ![Before and after: the torture test with a copper teapot, a black frame and a flat green cube (left), and a marble teapot, a wooden frame and an absorbing glass cube (right)](docs/images/changelog/torture-materials.png)

  Look at the teapot's gray veins, the wood grain round the mirror, and the cube: deeper green
  where light crosses more glass.
- **`scenes/solid_textures_test.ray`** shows both: a white marble sphere, a green marble cube, a
  wooden log and floor, a granite wall, the three slabs and the amber ball. New tests:
  `render_solid_textures` (with a reference render and a README preview),
  `roundtrip_solid_textures_test`, and `network_solid_textures`, which must match threads byte for
  byte. `format_test` checks that the noise is smooth, within about −1 to 1 and 0 on its lattice;
  that each seed gives its own noise and always the same one; that the color ramp blends evenly;
  that every texture round-trips through JSON; and that bad settings are reported. It also checks
  that a ray through a glass ball keeps exactly `attenuationColor` to the power of its chord over
  `attenuationDistance`.

## [2026-alpha-0.5] - 2026-11-20

Faster: plain value types for vectors and colors, a bounding volume hierarchy over the scene,
shadow rays that stop at the first hit, and lens tables worked out once and saved with the scene.
Every render is byte for byte what it was; the full-size torture test takes about a minute on a
Release build. Plus the `adaptive` sampler.

### Added

- **The `adaptive` sampler** (`--aa adaptive`, or `{ "type": "adaptive" }` in a scene): each
  pixel gets at least `min` rays (default 9), then more, one at a time, until the standard error of
  its brightness is under `threshold` (default 0.01) or it has `max` (default 64). Rays follow the
  R2 low-discrepancy sequence, shifted for each pixel by a hash, so any number of them is spread
  evenly over the pixel and over a real lens, and the image is reproducible and the same in every
  task. On the torture test at 320×240, measured against a 144-ray render: the scene's 5×5
  stratified sampler (75 rays a pixel with the three channels) has an RMS error of 2.54 levels;
  `adaptive` has 2.23 with 18% fewer rays, and with `min` 16, 1.84 with the same rays. New tests:
  `render_sampler_adaptive` (with a reference render and a README preview) and
  `network_sampler_adaptive`, which must match threads byte for byte. Samplers now get each ray's
  color before making the next, which `CTileTracer` always did.
- **A lens's exit pupil tables are worked out before the scene is sent, and saved with it.** The
  master works them out once, on every core, when the scene is loaded; they go into the lens's
  record (`exitPupil`: the tables as exact base64 doubles, and a fingerprint of the setup they are
  for), so tasks and engines use them instead of working them out each, and a saved scene loads
  without working them out. If the lens, focus, aperture or sensor changes, the fingerprint no
  longer matches and they are worked out again. `lens_test`, `chromatic_test` and `torture_test`
  are re-saved with theirs; renders are byte for byte the same.

### Changed

- **Faster tracing, with every render byte for byte the same.** On a Release build, the torture
  test at 320×240 went from about 13.0 s to 6.7 s, and `scan_test` with 9 random rays a pixel at
  1280×960 from 7.3 s to 5.4 s:
  - `CVector` and `CColor` are plain value types: no virtual functions, and their small members
    are inline, so the compiler can keep them in registers. The biggest single gain.
  - The scene keeps its objects in a bounding volume hierarchy: each object reports a bounding box
    (`CObjects::GetBounds`; planes have none and are tested every time), and a ray tests only the
    objects in boxes it passes through. Of objects hit at exactly the same distance, the first in
    the scene wins, as when every object was tested in order; `format_test` checks that the tree
    and the plain loop find exactly the same hits and shadows.
  - Shadow rays stop at the first thing in the way: a mesh stops at the first face it finds in its
    tree (`CObjects::Occludes`), and the object that blocked the last shadow ray is tried first.
  - A chromatic lens no longer traces the sampler's own ray through the glass, which was only used
    for where the sample falls; it is made again for each color anyway (`CCamera::SetAimOnly`).
  - A lens works out the index of each glass in each color once, instead of in every step of every
    ray.
- `scenes/README.md` no longer says reflected rays keep their parent's influence: they carry it
  times their material's share, so rays off weakly reflective surfaces stop after a few bounces.
- `CLAUDE.md`: time renders with a Release build; bounded objects override `GetBounds`, and objects
  made of parts `Occludes`.

### Fixed

- Release builds warned that parts of a task's events and inbox messages might be used
  uninitialized when copied; they now start at defaults.

## [2026-alpha-0.4.1] - 2026-11-20

Real lenses cast blocked rays again, so lens renders have almost no grain from the lens.

### Changed

- **A real lens casts a blocked ray again instead of letting it go black, so lens renders have
  almost no grain from the lens.** Whether a ray gets through the lens depends only on the lens
  and where it starts on the sensor, never on the scene. So the lens now works out, once, the share
  of rays aimed at its rear element that get through, at each ring of the sensor and in each color.
  A ray the lens blocks is cast again at another point in the exit pupil (up to 64 times, at
  points hashed from the first, so every run and every task makes the same ones) until one gets
  through, and that ray counts for the share. Brightness and vignetting are as before, but how
  many of a pixel's rays happened to be blocked no longer shows as grain.
  - On a plain grey background through the double Gauss, the difference between neighboring
    pixels falls from 0.022 (2026-alpha-0.4) and 0.038 (2026-alpha-0.3) to 0.002. Brightness
    matches a 1,024-ray render of the old sampling within 0.3% (1.2% in the corners, within that
    render's own noise).
  - The share is measured on the same grid as the exposure, so the center of the image is exactly
    as bright as through a pinhole.
  - The `lens_test` and `torture_test` references are regenerated, with their previews in the
    README; `chromatic_test`, whose lens blocks nothing in its frame, is unchanged, as is every
    render without a lens. What grain is left comes from the scene: out-of-focus edges and
    textures blending.

    ![Before and after casting blocked rays again: lens_test, the torture test, and a full-size crop of it](docs/images/changelog/recast.png)

## [2026-alpha-0.4] - 2026-11-19

New shapes and a new way to share out the work: cylinders, cones and boxes; meshes with a bounding
volume hierarchy; a torture test of everything through a real lens; PNG and high dynamic range
output; and tasks that pull tiles of the image, so camera rays are made in parallel and engines can
come and go.

### Added

- **Cylinders, cones and boxes.** Three new object types for scene files (documented in
  `scenes/README.md`):
  - `cylinder`: `base`, `top` and `radius`.
  - `cone`: `base`, `top`, `baseRadius` and `topRadius` (default 0, a point). A cone with a
    `topRadius` is truncated.
  - `box`: `center`, `size` (one number for a cube, or `[x, y, z]`) and `rotation` (degrees about
    x, then y, then z).

  Cylinders and cones point any way, from `base` to `top`. They are capped by default; with
  `"capped": false` they are open shells that can be seen into and shadow and reflect themselves
  inside. All three have their own surface coordinates for textures.

  The new sample scene `scenes/primitives_test.ray` has its own reference render and tests:
  `render_primitives`, `roundtrip_primitives_test`, and `network_primitives`, which must match the
  threaded render byte for byte. Its preview is in the README's expected-output table. Look for:
  - the inside of the open yellow tube, dark except where light falls through its open end;
  - the image texture upright on each face of the cube and wrapped around the truncated cone, with
    the grid filling its top cap;
  - the floor seen through the turned glass box, bent by refraction.

  ![The new primitives_test scene](docs/images/tests/primitives_test.png)

  `format_test` checks where rays hit each shape and the normals there, open and capped ends, rays
  from inside, surface coordinates, and the errors for bad sizes and radii. No existing render
  changes. The scene editor shows the new objects in its tree but has no dialogs to add or edit
  them yet.

- **Torture test.** `scenes/torture_test.ray` uses everything at once: every kind of object (sphere,
  plane, cylinder, cone, box, triangle, and the teapot mesh), mirrors, glass, image and checker
  textures, a reflective floor and a black-framed mirror wall, seen through the double Gauss lens with a 5×5
  stratified sampler (25 rays a pixel, each traced in red, green and blue) at 1024×768. The
  `render_torture` test renders it at 160×120 and compares it with a new reference render, which is
  in the README's expected-output table; `roundtrip_torture_test` loads and saves it. The lens
  focuses on the teapot at f/2 with a scene unit of 5 cm, so the depth of field is shallow. Look
  for the teapot and the scene behind the camera in the mirror wall, blurred because their light
  travels farther than the teapot's; the floor bent through the glass sphere and the green glass cube; and the
  nearest checks of the floor out of focus. The grain is the lens's: each of the
  25 rays takes a different path through the glass.

  ![The torture test at 1024x768](docs/images/torture_test.png)

- **PNG and high dynamic range output.** `raythrower --png <file>` writes the image as a PNG, and
  `--hdr <file>` writes every pixel's linear, unclipped color as a Radiance HDR (`.hdr`, RGBE)
  file, so highlights brighter than white survive for image editors, Blender and HDR viewers. Both
  are written by the new `CImageExport`, with a small deflate encoder of its own for PNG, so no
  zlib is needed. The raw format is still what the trace writes.
- **The `hdr` color model** (`--color hdr`, or `{ "type": "hdr", "exposure": 1 }` in a scene):
  colors scaled by `exposure` go unclipped into `--hdr` files, and 8-bit files get them clipped
  and sRGB-encoded. Color models now give the linear color for HDR files (`ToLinear`); `reinhard`'s
  is scaled by its exposure, and the others' is the traced color as it is.
- New tests: `render_color_hdr` (with a reference render, the same as `srgb` at exposure 1), unit
  checks of RGBE encoding and the `hdr` model, and `image_output`, which renders `scan_test` with
  `--png` and `--hdr` and decodes both with `tools/check_image.py`. The PNG must match the raw
  image exactly, and the HDR file within RGBE's precision, with colors above 1. It runs when CMake
  finds Python 3.

### Changed

- **Real lenses aim their rays at the exit pupil, so lens renders are less grainy.** A ray from the
  sensor used to go toward any point of the lens's rear element, and many were blocked by the
  aperture stop, especially toward the edges where the lens vignettes: blocked rays come back
  black, and how many varied from pixel to pixel. Now, for each of 64 rings of the sensor out from
  the center, the lens works out the exit pupil: an ellipse on the rear element around everywhere
  light gets through, found by tracing a grid of rays (in every color, for a chromatic lens). Rays
  are aimed only inside it, and each counts for the ellipse's share of the rear element
  (`CRay::GetWeight`), so the image is exactly as bright as before on average.
  - On a plain grey background through the double Gauss, the pixel-to-pixel noise falls by about a
    quarter at the center and nearly 40% at the edges and corners. `lens_test` is about 37%
    smoother, and the torture test about 27%. Average brightness is unchanged (within 0.3%).
  - When light gets through the whole rear element (as in `chromatic_test`, whose last surface is
    the stop), the lens aims at all of it, as before.
  - The rings are worked out the first time a ray needs them, and shared by every copy of the lens
    in the program, so the worker threads split the work and the master does none.
  - The `lens_test`, `chromatic_test` and `torture_test` references are regenerated, with their
    previews in the README; no other render changes. Look for the finer grain across the frame and
    the cleaner edges:

    ![Before and after exit-pupil sampling: lens_test, chromatic_test, the torture test, and a full-size crop of it](docs/images/changelog/exit-pupil.png)
- **Tasks pull tiles of the image instead of being dealt batches of rays.** The master splits the
  image into tiles (32×32 pixels, `raythrower --tile` to change it) and keeps them in a queue. Each
  task, a thread or a networked `rayengine`, asks for a tile and traces it with the new
  `CTileTracer`: it runs the camera's pixel sampler for the tile, makes the camera rays itself
  (through the lens, once a channel for a chromatic one), traces them, and sends back the tile's
  linear pixels, then asks for the next. Faster tasks take more tiles.
  - Making camera rays was the master's job, on one thread. With a real lens that held the torture
    test to about 3 of 16 cores. Now it takes 3 min 46 s at 1024×768 instead of 13 min 44 s,
    using about 10 cores.
  - Renders are byte for byte what they were: all 36 reference renders, and the full-size torture
    test, match. Samplers sum in fixed point, and a ray two tiles share (a corner of the `corners`
    sampler) is traced by both and comes out the same.
  - **Engines can come and go.** With `--listen`, `-N` engines must connect before tracing starts,
    but more can connect at any time, even mid-render: they get the scene and start asking for
    tiles. An engine that drops out has its unfinished tiles handed to the others. A trace fails
    only if every engine is gone and none connects within the `--timeout`. The new `engine_churn`
    test runs one engine that quits after its first tile and another that joins mid-trace, and the
    image must match threads exactly.
  - The wire protocol is version 4: `MESSAGE_REQUEST`, `MESSAGE_TILE`, `MESSAGE_PIXELS` (the
    pixels as doubles) and `MESSAGE_STOP` replace the ray and color batches. Engines and masters
    must come from the same release.
  - `CPixelSampler::Begin` takes a tile (`left, top, width, height`), in image coordinates;
    `BeginImage( width, height )` samples a whole image. `CPixelWriter` is now an interface:
    `CImageWriter` writes the image through the color model, and `CTileTracer` keeps a tile's
    pixels.
- **Meshes are much faster.** A mesh keeps its faces in a bounding volume hierarchy, so a ray tests
  only the faces in boxes it passes through instead of every face. The teapot renders about 30
  times faster (41 s to 1.4 s at 320×240 on 16 cores). Renders are byte for byte the same: of faces
  hit at exactly the same distance, the tree picks the first in the mesh, as testing every face in
  order did. `format_test` checks that the tree and the plain loop find exactly the same hits.
- `.gitignore` ignores `nohup.out`.

### Removed

- `raythrower --batch` (rays per batch), and `CRayArray` and `CColorArray`, which carried the
  batches; `--tile` sets the work unit's size instead.

### Fixed

- `texture_checks` failed on Windows: it wrote the texture's absolute path into the test scene
  with backslashes, which JSON reads as escapes. It now writes the path with forward slashes.

## [2026-alpha-0.3] - 2026-11-19

The ray tracing has resumed: a corrected camera, anti-aliasing, color models, real lenses with
chromatic aberration, meshes and the Utah teapot, textures, corrected materials, and scenes saved
as JSON inside IASLib resource archives.

### Added

- **JSON scene files.** Scenes are saved as human-readable JSON (version 3 of the `.ray` format,
  same extension) that can be written and edited by hand. Only the camera is required; anything
  left out takes its default. Unknown fields are errors, so misspellings are caught, and every error
  says where it is (`objects[0].materials[1]: unknown field "colour"`). The format is documented in
  `scenes/README.md`. It adds the background color, the camera's sampler and color model, and
  triangle and mesh options to what the binary format held, and keeps every 1997 setting, the
  never-implemented ones under `settings.reserved1997`. Saved numbers are rounded to six decimal
  places, and angles are in degrees. Built on IASLib's JSON support, which the build now turns on.
  - Each scene class reads and writes its own fields (`ToJson`/`FromJson`); the object and material
    registries give each class a JSON `type` name and can create one empty.
  - `CScene::GetLoadError` says why a load failed; a binary scene from 1996–1999 is recognized and
    reported as no longer readable.
- **Pixel samplers.** A camera owns a `CPixelSampler` that makes its primary rays and turns their
  colors into pixels, so new anti-aliasing or ray generation schemes are new subclasses attached
  with `CCamera::SetSampler` or named in the scene. `CTaskMaster::TraceScene` just runs the camera's
  sampler. Samplers are listed by name in `PixelSampler.cpp`.
  - `none` (`CCenterSampler`, the default): one ray through each pixel's center, as before.
  - `corners` (`CCornerSampler`): the first anti-aliasing mode. It traces five rays for each pixel,
    one at each corner and one at the center, and averages them. Each corner is traced once and
    shared by the four pixels around it, so it costs about two rays per pixel instead of five.
    Rays are tagged with their position on a doubled sample grid, so `rayengine` needs no change,
    and samples are summed in fixed point, so threads and networked engines produce identical
    images.
  - `box` (`CBoxSampler`): four rays per pixel, each halfway from the center to one of its corners,
    averaged equally.
  - `random` (`CRandomSampler`): a ray through the center worth half the pixel, and N rays at random
    points in the pixel sharing the other half (`samples`, N, from 1 to 1024, default 4). The points
    come from a hash of the pixel, ray number and `seed`, so images are reproducible and threads and
    engines agree.
  - `CPixelAccumulator`: the weighted, fixed-point, order-independent averaging the samplers share.
- **Texture mapping** (`CTexture`): any material can have a `texture` that gives its color at each
  point. Textures: `checker` (two colors, in surface coordinates or `solid` in the scene) and
  `image` (PPM P6/P3 or Ray raw files, bilinear, with `scale`, `offset`, `repeat` and `srgb`).
  Objects give surface coordinates (`CObjects::SurfaceCoords`): spheres by longitude and latitude,
  planes along two axes across them, triangles and meshes from new optional `uvs`. Image pixels
  travel inside the scene when it is copied to a task (`RayJson::CEmbedData`, base64 `pixels`),
  so networked engines need no files and still match threads exactly. Image files are read
  relative to the scene file, and saved relative to wherever the scene is saved (absolute when
  the two share only the root). New `scenes/texture_test.ray` and `scenes/textures/uv_grid.ppm`.
  - UV mapper: a mesh without `uvs` can have a `uvMap` (`planar`, `cylindrical`, `spherical` or
    `box`, about an `axis`, with an optional `center` and `scale`) that works out each face's
    corner coordinates when the mesh is built. Faces across a cylindrical or spherical seam are
    unwrapped, and pole corners take their face's angle. New `scenes/teapot_textured.ray`.
- **Real camera lenses** (`CLens`): a camera can have a lens prescription, spherical glass surfaces
  listed front to back as in lens patents, and rays are traced from the sensor back through each
  surface out into the scene. Rays the barrel or the aperture stop blocks go dark, so depth of
  field, vignetting, and the lens's aberrations and distortion come from the optics. The lens's
  focal length and widest f-number are worked out, the sensor is placed to focus at
  `focusDistance` (default: the look-at point), and exposure is set so the image center is as bright
  as through a pinhole (`CPixelWriter` now takes an exposure). The ray's position in its pixel picks
  where it crosses the lens, with a per-pixel shift, so evenly spread rays sample the lens evenly.
  Built in: `double-gauss-50mm` (50 mm f/2, US 2,673,491, as tabulated by Kolb et al. 1995). The
  example prescription in the design brief this started from turned out to be a diverging lens
  (focal length −16 mm), which can't form an image; such lenses are rejected with that message.
  A camera without a lens is the same pinhole as before, byte for byte. New `scenes/lens_test.ray`.
  - Chromatic aberration: surfaces take an `abbe` number for the glass that follows. Each glass's
    index is fitted to a Cauchy curve through its d-line index and Abbe number, and each sample is
    traced three times, red at the C line (656.3 nm), green at the d line (587.6 nm) and blue at
    the F line (486.1 nm), each keeping only its channel; the task master recombines them. The
    channel travels in the top bits of the ray's tag, so engines and the wire format are
    unchanged and networked images still match threads. Green uses the index as given, so focus
    and exposure are unchanged. `"chromatic": false` turns it off. The built-in double Gauss has
    its Abbe numbers (from Kolb's table; three of its glasses match catalogue SF15, F5 and LaF3),
    and is corrected for color (red and blue focal lengths within 0.06 mm), so its fringes are
    faint; new `scenes/chromatic_test.ray` uses an uncorrected BK7 singlet, whose color focus
    shift (0.75 mm) matches the textbook f/V.
  - `stratified` sampler (`CStratifiedSampler`): an n×n grid of equally weighted rays per pixel,
    each at a random point in its cell (`grid`, n, default 4; `seed`). Equal weights matter with a
    lens, where the `random` sampler's half-weight center ray makes every blocked ray a dark speck.
- **Triangles work out their own normals** from their corners, instead of storing them, by the
  edge traversal in the 1997 code (which had been commented out) with its order reversed:
  (V2 − V1) × (V3 − V2). The 1997 order made faces of standard models such as OBJ files face
  inward. The corner order decides the front; the two 1997 triangle scenes had their corners
  reordered to keep facing the camera, and are double-sided. New `doubleSided` (a single-sided
  triangle can't be hit from behind; a double-sided one is shaded from the side it is seen from)
  and `smooth` flags. Optional `normals` override the corner normals smooth shading blends (and
  make the triangle smooth); `triangle2.ray` keeps its 1997 tilted normals this way, which are what
  light it.
- **Meshes** (`"type": "mesh"`, `CMesh`): a vertex list and faces given as vertex indices. The
  faces are `CTriangle`s that share the mesh's materials. In a smooth mesh each vertex's normal is
  the sum of the normals of the faces around it, and faces blend their corner normals by barycentric
  weights, so shared edges show no seam; optional `normals`, one per vertex, replace the summed
  ones. Rays that miss the mesh's bounding box skip all its faces,
  and rays leaving a face skip only that face (`CObjects::Intersect` now takes the object to
  ignore), so a mesh shadows and reflects itself.
- **The Utah teapot** (`scenes/teapot.ray`, 6,320 triangles), and `tools/obj_to_ray.py`, which turns
  OBJ models into meshes (with options to smooth, weld repeated vertices, and flip the odd model
  wound the other way).
- **Color models.** A camera owns a `CColorModel` that turns each pixel's linear color, of any
  brightness, into the image's bytes; samplers now hand over unclipped colors through a
  `CPixelWriter`. Color models are listed by name in `ColorModel.cpp`:
  - `clip` (the default): clip at 1 and truncate, exactly as the 1997 image writer did;
  - `gamma`: gamma encoding, with a `gamma` setting (default 2.2);
  - `srgb`: the sRGB curve;
  - `reinhard`: Reinhard tone mapping on luminance, with `exposure` and `white` settings, then sRGB.
- `raythrower` options: `--aa <sampler>`, `--color <model>` and `--size <w>x<h>` override the
  scene's choices, and `--aa`/`--color` take settings after a colon (`--aa random:samples=9`); `--save <file>` saves the scene and exits; `--help` lists the samplers and color
  models. It prints the rays traced and the rays per pixel, and says why a scene couldn't be read.
  The scene editor does not offer a sampler or color model yet.
- An "Expected output" section in the README showing every reference render the tests compare
  against, labelled with its test, and `tools/render_previews.py`, which makes those previews
  (`docs/images/tests/`) from `tests/reference/renders.tar.gz`.
- `CRay::CreateRayAt`, which casts a primary ray through any point on the image,
  `CImageFile::PutRGB`, `CVector::Cross`, and `CScene::GetObjectAt`.
- Tests: material checks (Snell's law in and out, total internal reflection, specular edge
  cases and default index, and that shading a plane leaves it unchanged); UV mapper checks (each projection, seam unwrapping, poles, errors) and a textured teapot
  render; a texture render and a networked one (the image must reach the engines), texture checks
  (embedding, loading a copy without the files, errors); lens and chromatic renders (and a networked chromatic render, which must match threads),
  checks of each lens's color focus shifts, a lens render and lens checks (the double Gauss's focal length, f-number and focus, and
  the errors for unusable lenses), the stratified sampler, four scenes rendered with corner
  anti-aliasing, one with `box` and with `random` at 2, 4
  and 9 rays (and networked `random`, which must match threads exactly), one with each color
  model, and the teapot
  (new references in `tests/reference/renders.tar.gz`); networked engines must match threads with
  anti-aliasing on; rays and colors must cross the wire exactly; JSON scenes with mistakes must
  fail with the right message and leave the scene untouched; triangles must cull back faces only
  when single-sided, and smooth mesh faces must agree on the normal along a shared edge.

### Changed

- **Saved scenes are IASLib resource archives** (`CResourceFile`): `scene.json` and every file the
  scene uses (texture images), under the names the scene gives them, so one `.ray` file carries
  the whole scene. Saving writes beside the file and renames it into place, so a failed save
  leaves the old file. Plain JSON `.ray` files still load (the sample scenes stay JSON), and new
  `CScene::SaveJson` / `raythrower --save-json` writes JSON for editing by hand, extracting the
  files beside it. Files a scene names are read from its archive first (`RayJson::ReadSceneFile`)
  and collected into it when saving (`RayJson::SceneFileName`); names outside the scene's
  directory are stored under `files/`. The round-trip tests compare scenes rather than bytes,
  since archives record when they were written.
- The default trace depth is 16 levels (`settings.maximumDepth`; the camera ray is level 1, so 15
  reflections or refractions), up from the 1997 default of 15, and every sample scene uses it.
  The deepest bounces in `reflection_test`, `reflect_me` and `reflect_test2` change their renders;
  their references and the README gallery were regenerated.
- The sample scenes in `scenes/` are now JSON, converted from the 1997 originals and rounded to six
  decimal places. Rounding moves a few pixels by one level, and `triangle2` now blends its corner
  normals by barycentric weights instead of the 1997 inverse-distance weighting; the reference
  renders were regenerated.
- The wire protocol (version 3): the scene is sent as SJSON, IASLib's compact binary JSON; ray
  and color batches are fixed-layout big-endian IEEE-754 doubles (72 bytes a ray, 32 a color),
  which `RayWire::ToReadable` can show as JSON. SJSON was tried for the batches too, in stream,
  stateless and schema modes, but it costs 30–45 µs a ray to encode, more than tracing it.
  Engines and masters must be from the same release.
- IASLib is pinned to commit `de038c0`, which added SJSON, until it tags a release with it. SJSON's
  optional LZ4 and zstd support is left off, so every build is the same.
- `CTaskMaster::ColorsFromTasks` no longer takes the image; colors go to the camera's sampler.
- `CImageFile::PutPoint` writes through the `clip` color model, which also treats negative
  channels as 0 (the 1997 code converted them to a byte without a check, which is undefined).
- `CRay::CreateRay( CScene &, ... )` takes pixel coordinates instead of angles, and the camera
  works out the direction (`CCamera::RayDirection`). `CTaskMaster::TraceScene` and the GUI preview
  loop no longer do their own angle stepping.

### Removed

- The binary scene format of 1996–1999 and `CRayStream`, with every class's stream operators,
  stream constructors and `Serialize`, and the registries' stream factories. The 1997 binary
  sample scenes are gone too (they are in the history); two of them could never be read anyway.

### Fixed

- **Material math** (`CTransparentMaterial`, `CSpecularMaterial`, `CReflectiveMaterial`,
  `CObjects::BaseShade`):
  - Refraction had the wrong sign on the cos θ₁ term of Snell's law in vector form, entering and
    leaving glass, so rays bent far too much (45° into glass of index 1.5 came out at 14.5°
    instead of 28.1°), and past the critical angle it took the square root of a negative number.
    It now follows T = ηV + (η cos θ₁ − cos θ₂)N with the normal facing the ray, and reflects
    inside on total internal reflection (`CTransparentMaterial::Refract`).
  - Materials were handed the surface normal by reference, and the diffuse material turned it
    toward the viewer in place. For a plane that was the plane's own normal, so shading changed
    the scene (and could flip a plane's texture), depending on which side the last ray came from.
    Each material now works on its own copies.
  - The specular (Cook–Torrance) material didn't turn the normal toward the viewer, and divided
    0 by 0 (a NaN, a black pixel) on the shadow line; it now has no highlight for lights behind or
    surfaces edge on. Its default index of refraction was 1.0, which makes the Fresnel term 0 and
    the highlight vanish; it is 1.5 now (the teapot shows its highlight). An index below 1 is
    treated as total reflection rather than a NaN.
  - Reflected and refracted rays now carry their parent's influence times the material's, so
    `depthThreshold` prunes rays that no longer matter; `BaseShade` no longer also multiplies its
    result by the ray's influence, which would have counted it again at every bounce.
  - The reflection direction is computed directly as V − 2(N·V)N (it was right, by way of a
    detour).
  - `scan_test`'s glass sphere had index 1.0, which looked like glass only because of the
    refraction bug; it, `lens_test`, `chromatic_test` and `axis_test`'s partly transparent sphere
    now use 1.5. Every reference render, preview and README image was regenerated.

    ![Before and after: scan_test's glass sphere, axis_test, triangle2, and the teapot's specular highlight](docs/images/changelog/material-fixes.png)

    Before: the 1997 refraction, and (teapot) the old default specular index of 1.0, which left no
    highlight. After: Snell's law, glass at 1.5, and the highlight.

- Camera rays were distorted. The camera was aimed by adding the difference between its view
  direction and +Z to every ray instead of rotating the rays, which sheared the image: spheres came
  out as tilted ellipses, horizons curved, a camera looking straight down saw a smear, and a camera
  turned to face -Z rendered a mirror image. Pixels were also spaced by equal angles rather than
  evenly across a flat image plane, and were not square unless the scene's two angles happened to
  match the image's shape.
  Primary rays now come from a proper camera basis (view direction, right, and up, from the stored
  up vector, with a fallback when looking straight up or down) through the center of each pixel on
  a flat image plane. The vertical angle sets the field of view and the horizontal one follows from
  the image's width and height, so pixels are square.
  The reference renders in `tests/reference/renders.tar.gz` and the README images were regenerated.

  ![Before and after the camera fix: axis_test, xxx and reflect_me](docs/images/changelog/camera-fix.png)

  Before: the six spheres of `axis_test` sheared into ellipses, `xxx` mirrored, and `reflect_me`
  (a camera looking straight down) smeared. After: round spheres, the right way round, and the
  view down onto two spheres.

## [0.2.0] - 2026-11-19

The code builds again, on Windows, Linux, and macOS, on top of
[IASLib v3.0](https://github.com/jnaujok/IASLib). The ray tracing logic is unchanged.

### Added

- `core/`: one ray tracing library shared by every program. It replaces the separate, drifted
  copies in the MFC application and in `rayengine/`.
- A TCP transport built on IASLib sockets (`CSocketTaskMaster`, `CSocketTaskSlave`, and a framed
  wire protocol in `WireProtocol.h`). It takes the place of PVM.
- `raythrower` options: `--listen [port]` to trace with networked engines, `--spawn` to start
  local `rayengine` processes, `-o` to choose the image file, `--ppm` to also write a viewable
  PPM image, `--batch`, `--timeout`, and `--tasks` (the original `-Nxxx` still works).
  Without `--listen` it traces with threads.
- `CScene::Load` and `CScene::Save`. `Load` reads version-1 and transitional scene files with the
  pre-April-1997 number format, which the 1997 code itself could no longer read.
- Regression tests (`ctest`): every readable sample scene is rendered and compared with a reference
  render, networked engines must produce exactly what threads produce, scene files must survive a
  load and save, and the serialization format is checked byte for byte.
- GitHub Actions CI: Linux (GCC and Clang), macOS, and Windows builds with tests, a Linux
  AddressSanitizer/UBSan job, and a Windows build of the MFC scene editor.
- `scenes/README.md`, describing the sample scenes and the file format history.

### Changed

- Uses IASLib's `CString`, streams, threads, mutexes, semaphores, sockets, and `TArray` instead of
  the project's own early versions of them. `CStream` is now `CRayStream`, a reader and writer of the
  original binary format on top of any IASLib stream.
- `CRayArray`, `CColorArray`, `CObjectsArray`, `CLightArray`, and `CMaterialList` are built on
  IASLib's `TArray`, with the same interfaces and ownership rules.
- `CThreadTaskMaster` uses IASLib threads and in-memory queues instead of named pipes and Win32
  semaphores. It is portable and part of the core.
- `raythrower` reuses `CTaskMaster::TraceScene` instead of its own copy of the ray loop.
- Built with CMake as C++17. IASLib is fetched by CMake at `v3.0.0`.
- The core's GUI code is keyed on `RAY_GUI` instead of `_WINDOWS`, which CMake defines for every
  MSVC build.
- The Visual C++ 6 project files moved to `legacy/msvc6/`; they cannot build the new code.

### Removed

- PVM: the bundled PVM 3 headers and libraries, `CPVMStream`, `CPVMTaskMaster`, `CPVMTaskSlave`.
- `CFileStream`, `CPipeStream`, `CSem`, and the engine's own `CString`, all replaced by IASLib.
- The Unix Makefiles, replaced by CMake.
- Stubs that were never compiled: `TraceEngine.h`, `TraceThread`, `ImageStream.h`, `DynMaterial.h`.

### Fixed

- `CRay` never initialized or copied its reflection depth, so every ray started with a garbage depth.
  Traces were not repeatable, and the reflection scenes overflowed the stack.
- The object class registry existed only in the GUI; every other program crashed on its first object.
- Loading a scene leaked the camera object. An unknown object class now fails the load instead of
  reading the rest of the file as garbage.
- `CObjectsArray::Delete` and `CLightArray::Delete` never reduced the count, leaving a duplicate
  pointer that was later deleted twice. `SetX`, `SetY`, and `SetInfluence` tested their index with
  `||`, so they wrote out of bounds. The arrays freed `new[]` memory with `delete`.
- `CMaterial` had no virtual destructor, though materials are deleted through it.
- `CColorArray( int )` and the other array constructors were implicit, so
  `oStream << COLOR_ARRAY_VERSION` called the array serializer.
- `CImageFile` called `setvbuf` on a file that failed to open, and wrote pixels at any coordinates
  it was given; out-of-range pixels are now refused.
- The ray thrower never sent the last partial batch of rays (so it waited forever), never told the
  engines to stop, dropped pixels after the first reply from each engine, and could divide by zero.
- `CTaskMaster::TraceScene` overflowed its progress value on images over about 200,000 pixels and
  left `IsTracing()` true after a trace.
- Scene files saved over a larger file kept the old file's tail. The GUI ignored load failures, and
  its display thread opened the image for writing while the tracer was writing it.
- Long values are written as 4 bytes on every platform, so scene files from 64-bit Linux and macOS
  match those from Windows.

## [0.1.0] - 2026-11-19

The first public import of the 1997 code base, reorganized into a modern project layout.
The ray tracing code itself is unchanged.

### Added

- `CMakeLists.txt` that builds the MFC scene editor with MSVC, plus the Unix PVM engine and thrower
  behind the `RAY_BUILD_PVM_TOOLS` option.
- Project documentation: `README.md`, `rayengine/README.md`, `raythrower/README.md`,
  `CHANGELOG.md`, `CONTRIBUTING.md`, `CLAUDE.md`.
- Apache License 2.0 (`LICENSE`) and `NOTICE`.
- `.gitignore`, `.gitattributes`, and a pull request template saying outside contributions
  aren't accepted.

### Changed

- Renamed every file from DOS upper case to modern names. Sources and headers are PascalCase after
  their class name (`ADDPLANEDLG.CPP` → `AddPlaneDlg.cpp`, `TRANSFRM.H` → `Transform.h`), and data
  files are snake_case (`INSIDE SPHERE.RAY` → `inside_sphere.ray`).
- Moved the code into `src/` and `include/` directories:
  - the MFC application went to the root `src/` and `include/`,
  - `RAYENGINE/` → `rayengine/{src,include}`,
  - `RAYTHROWER/` → `raythrower/{src,include}`.
- Renamed `.CC` sources to `.cpp`. RayThrower's Unix entry point is now `main_unix.cpp`.
- Moved resources to `res/`, WinHelp sources to `help/`, scenes to `scenes/`, PVM headers and
  libraries to `third_party/pvm3/`, and Visual C++ 4/5 project files to `legacy/`.
- Rewrote `#include` directives to match the new file names exactly, so they work on
  case-sensitive file systems. Replaced the path-relative includes
  (`"rayengine\cstring.h"`, `"..\globals.h"`, `"../pvm3/include/pvm3.h"`) with include paths.
- Pointed `Ray.dsp`, `Ray.clw`, `Ray.rc`, `makehelp.bat`, and the Unix Makefiles at the new paths.
- Converted sources, headers, resource scripts, and docs from CRLF to LF line endings. The Visual
  C++ 6 project files and batch files stay CRLF.

### Known issues

- The Unix sources don't compile cleanly with a modern GCC or Clang, mainly because `TRUE` and
  `FALSE` are expected to come from the platform headers.
- `rayengine/Makefile` doesn't list `src/PVMStream.cpp`. The original Makefile left it out too.
- The project files under `legacy/` still contain the pre-reorganization paths.

## [Historical] - 1996-1999

Original development of the distributed ray tracer: the MFC scene editor, the PVM ray engine, and
the ray thrower, built with Visual C++ 4–6 on Windows and with `make` on Unix.

[Unreleased]: https://github.com/jnaujok/Ray/compare/2026-alpha-0.7...HEAD
[2026-alpha-0.7]: https://github.com/jnaujok/Ray/compare/2026-alpha-0.6...2026-alpha-0.7
[2026-alpha-0.6]: https://github.com/jnaujok/Ray/compare/2026-alpha-0.5...2026-alpha-0.6
[2026-alpha-0.5]: https://github.com/jnaujok/Ray/compare/2026-alpha-0.4.1...2026-alpha-0.5
[2026-alpha-0.4.1]: https://github.com/jnaujok/Ray/compare/2026-alpha-0.4...2026-alpha-0.4.1
[2026-alpha-0.4]: https://github.com/jnaujok/Ray/compare/2026-alpha-0.3...2026-alpha-0.4
[2026-alpha-0.3]: https://github.com/jnaujok/Ray/compare/v0.2.0...2026-alpha-0.3
[0.2.0]: https://github.com/jnaujok/Ray/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/jnaujok/Ray/releases/tag/v0.1.0
