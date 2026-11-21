<div align="center">

# Ray

**A distributed ray tracer from 1997: an MFC scene editor on the front end and a farm of tracing engines on the back end.**

[![Build and test](https://github.com/jnaujok/Ray/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/jnaujok/Ray/actions/workflows/build.yml)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg?logo=cplusplus&logoColor=white)](#building)
[![CMake](https://img.shields.io/badge/CMake-3.18%2B-064F8C.svg?logo=cmake&logoColor=white)](#building)
[![Built on IASLib](https://img.shields.io/badge/built%20on-IASLib%203-8A2BE2.svg)](https://github.com/jnaujok/IASLib)
[![Since 1997](https://img.shields.io/badge/since-1997-yellow.svg)](#about)

[![Linux](https://img.shields.io/badge/Linux-GCC%20%7C%20Clang-FCC624.svg?logo=linux&logoColor=black)](#platforms)
[![macOS](https://img.shields.io/badge/macOS-Apple%20Clang-000000.svg?logo=apple&logoColor=white)](#platforms)
[![Windows](https://img.shields.io/badge/Windows-MSVC-0078D6.svg?logo=windows&logoColor=white)](#platforms)
[![Tested with ASan and UBSan](https://img.shields.io/badge/tested%20with-ASan%20%7C%20UBSan-success.svg)](#testing)
[![Contributions: closed](https://img.shields.io/badge/contributions-closed-red.svg)](CONTRIBUTING.md)
[![Last commit](https://img.shields.io/github/last-commit/jnaujok/Ray.svg)](https://github.com/jnaujok/Ray/commits/main)

<img src="docs/images/scan_test.png" alt="scan_test.ray: reflective and transparent spheres over a plane, anti-aliased" width="480">

</div>

---

## Table of contents

- [About](#about)
- [Features](#features)
- [How it works](#how-it-works)
- [Platforms](#platforms)
- [Building](#building)
- [Usage](#usage)
- [Testing](#testing)
  - [Expected output](#expected-output)
- [Repository layout](#repository-layout)
- [Scene files](#scene-files)
- [Project status](#project-status)
- [Changelog](#changelog)
- [Contributing](#contributing)
- [License](#license)

## About

Ray is a ray tracer written in 1996–1997 that splits rendering across a network of machines.
You build and edit scenes in a scene editor (the cross-platform `ray-editor`, or the original
Windows MFC application), and the image is traced in parallel by engine processes on any number of
machines, or by worker threads on one.

It was originally written for [PVM](https://www.netlib.org/pvm3/) (Parallel Virtual Machine) on
a cluster of Unix workstations. The cross-platform library written for it grew into
[IASLib](https://github.com/jnaujok/IASLib), and the ray tracer now runs on IASLib 3: PVM is
replaced by plain TCP, and the command-line programs build on Windows, Linux, and macOS.
The ray tracing itself is the 1997 code.

<div align="center">
<img src="docs/images/gallery.png" alt="Anti-aliased renders of six of the sample scenes, including the Utah teapot" width="720">
</div>

## Features

- **Primitives:** spheres, planes, cylinders, cones, boxes, triangles, and triangle meshes, typed
  into the scene or loaded from STL (binary, ASCII, or colored) and OBJ model files.
  Cylinders and cones run between any two points, capped or open (an open one can be seen into and
  shadows itself inside); a cone can be cut off flat at the top, and a box can be any size along
  each axis and turned any way. A triangle's normal comes from
  its corner order; it can be single- or double-sided, and flat or smooth, with corner normals that
  can be given to override the computed ones. A mesh shares vertices
  between faces, gives each vertex the sum of its faces' normals, and keeps its faces in a bounding
  volume hierarchy built by the surface area heuristic, so a ray tests only the few faces near its
  path, nearest first. Its faces are just three vertex indices each (about 50 bytes a face with the
  tree), and every thread's copy of a scene shares one copy of each mesh's shape, built once. The scene keeps its objects
  in one too (planes, which are infinite, are tested every time), and shadow rays stop at the
  first thing in the way.
- **Constructive solid geometry:** a `csg` object combines closed shapes (spheres, boxes, capped
  cylinders and cones, closed meshes, and other csgs) by union, intersection or difference: a lens
  blank from two balls, a bowl from a ball with a ball taken out, a die with drilled pips. It shades
  as one object with its own materials, or with each shape's own (`childMaterials`), shadows and
  reflects itself, and takes bump maps and textures like any object.

  <img src="docs/images/tests/csg_test.png" alt="A glass lens blank, a bowl blue outside and white inside, a gold capsule and a die with drilled black pips, all made by constructive solid geometry" width="320">

  <img src="docs/images/tests/primitives_test.png" alt="A capped cylinder, an open tube, a cone, a textured truncated cone, a textured cube, and a glass box on a checkered floor" width="320">
- **Lights:** point lights; spot lights, whose cone fades over a soft edge; directional lights
  (sunlight), the same everywhere; and area lights that cast soft shadows: a glowing ball
  (`sphereLight`), a glowing panel (`rectangleLight`), a spot light with a radius, and a sun with
  an angular size, whose shadows blur the farther they fall, each shaded from points spread over it.

  <img src="docs/images/tests/lights_test.png" alt="A warm spot with a hard edge on a box, a white spot with a soft edge on a ball, a blue spot with soft shadows on a cone, and a low sun casting long soft shadows" width="320">
- **Environment maps:** rays that leave the scene see a sky gradient or an HDR (or PNG) picture of
  the surroundings instead of a flat background, behind the scene, in mirrors and through glass,
  and it can light diffuse surfaces too (image-based lighting), smooth from a map of the sky's
  light worked out once, darker wherever shadow rays find the sky hidden. `tools/make_textures.py`
  makes an HDR sky for the tests.

  <img src="docs/images/tests/environment_test.png" alt="An HDR sky with clouds behind a mirror ball, a glass ball, a white ball and a red ball on a checkered floor, lit by the sky" width="320">
- **Indirect light:** light bounced between surfaces (`settings.indirect`), so a red wall tints the
  white floor beside it pink and shadows and ceilings get fill light. Matte surfaces gather light
  from the surfaces near them with rays spread from each sample's point in its pixel, so a
  stratified sampler smooths it, and every thread and engine gives the same image.

  <img src="docs/images/tests/bounce_test.png" alt="A white box room with a red left wall and a green right wall, a white block and ball, the walls' colors bleeding onto them" width="320">
- **Haze:** an atmosphere (`haze`) that far things fade into and that dims light on its way to a
  surface, the same everywhere or thinning with height into ground mist.

  <img src="docs/images/tests/haze_test.png" alt="An avenue of colored columns and balls fading into ground mist under a blue sky" width="320">
- **Materials:** ambient, diffuse, specular, reflective, and transparent, mixed per object by
  influence. Transparent materials can absorb light by how far it travels inside (Beer–Lambert),
  so thick glass is deeper in color than thin, and a glass ball darker through its middle than at
  its rim. Glass and mirrors can be rough: frosted glass blurs what is behind it, and a brushed or
  satin mirror blurs its reflection (GGX microfacets), best with a sampler that traces many rays
  a pixel. Glass with an Abbe number disperses: a ray splits into red, green and blue at it, each
  bent by its own index, so a flint glass prism or ball fringes what is seen through it with color.
  With the camera's `"dispersion": "spectral"` it splits instead into four single wavelengths,
  weighed by the CIE color matching functions and spread over the range by a per-pixel shuffle of
  wavelength strata, so over a pixel's samples the colors smear into a continuous rainbow (yellows
  and cyans too) instead of three bars.
  An optional sample clamp on the camera keeps a rare very bright ray (a "firefly") from showing
  as a lone speck.

  <img src="docs/images/tests/prism_test.png" alt="A flint glass prism and ball before white bars on a black wall, fanning them out into a spectrum" width="320">

  <img src="docs/images/tests/soft_test.png" alt="Clear, lightly frosted and heavily frosted glass panes before a striped poster, a mirror ball and a brushed gold ball, and soft shadows from a rectangle light and a sphere light" width="320">
- **Textures:** any material's color can come from a texture instead: a checkerboard, on the
  surface or solid (carved from a checkered block), an image file (PNG of any kind, PPM, or Ray
  raw) laid over the surface, or a solid noise texture: marble, wood, or plain noise and turbulence (granite, stone,
  clouds), made from Perlin gradient noise and a ramp of colors. Spheres, planes, cylinders, cones, and boxes have surface coordinates of their own; triangles and meshes take
  `uvs`, and a mesh without them can use a UV map (planar, cylindrical, spherical, or box
  projection) to work them out. Networked engines get the images inside the scene, so they need no
  files.

  <img src="docs/images/textures.png" alt="A checkered floor, an image texture on a wall and a sphere, and a solid checker sphere" width="320"> <img src="docs/images/teapot_textured.png" alt="The Utah teapot with an image texture, spherically UV-mapped" width="320">

  <img src="docs/images/solid_textures.png" alt="A white marble sphere, a green marble cube, a wooden log and floor before a granite wall, three green glass slabs that darken with thickness, and an amber glass ball" width="640">
- **Model files and printed plastic:** a mesh can load an STL or OBJ `file` and place it with a
  `scale` (or `fitSize`), `rotation`, `align` and `translation`; the file goes into the saved scene
  and, inside the scene, to networked engines. Colored STL files (the VisCAM/SolidView and
  Materialise Magics conventions) give each face its color through the `faceColors` texture. The
  `layers` texture draws the rounded 0.1 mm layer lines of a 3D print, as a color and as a bump map;
  a model's textures are looked up in its file's own coordinates (millimeters for a printable
  model), so the layers follow its print axis however it is turned. The sample scenes include
  [#3DBenchy](https://3dbenchy.com) (225,706 triangles).

  <img src="docs/images/tests/benchy_test.png" alt="3DBenchy in orange plastic, close up on its bow, with the layer lines of a print" width="320"> <img src="docs/images/tests/models_test.png" alt="A puzzle cube and a gem from colored STL files and a textured ring from an OBJ file on a checkered floor" width="320">
- **Bump and normal maps:** an object's `bump` tilts the normal it is shaded with, so it looks
  dented, grooved or rough while its shape stays smooth. A height map reads any texture as heights
  (a little noise makes hammered metal or orange peel; a wood or marble texture makes its grain
  stand out), and a normal map reads a tangent-space normal image in the common OpenGL convention.
  Every material on the object follows the bumps, highlights and reflections included.

  <img src="docs/images/tests/bump_test.png" alt="A brick wall with a PNG normal map, a hammered copper ball, an orange-peel ball, a box embossed with bricks by a height map, and a wood floor bumped by its grain" width="320">
- **Anti-aliasing:** the camera's primary rays come from a pluggable pixel sampler: `corners`
  (five rays per pixel, four of them corners shared with the neighbors, about two rays a pixel),
  `box` (four rays halfway from the center to each corner), `random` (a center ray worth half
  the pixel plus N random rays), `stratified` (an n×n grid of equally weighted rays, each at a
  random point in its cell; best for real lenses), or `adaptive` (at least 9 rays a pixel, then
  more only where they disagree: on the torture test, 28% less error than a 5×5 grid for the same
  rays). Random points are reproducible, so every run gives the same image.
- **Camera:** pinhole camera with position, look-at point, up vector, and vertical field of view;
  square pixels at any image size.
- **Real lenses:** give the camera a lens prescription (spherical glass surfaces, front to back, as
  in lens patents) and rays are traced from the sensor back through each element. Rays the barrel or
  aperture stop blocks go dark, so depth of field, vignetting, and the lens's own aberrations and
  distortion come out of the optics. Give the glasses Abbe numbers and red, green and blue are
  traced at their own wavelengths, for chromatic aberration (or, with spectral dispersion, four
  single wavelengths a sample, so the fringes smear into a rainbow). Focus and exposure are worked out
  automatically; a 50 mm f/2 double Gauss is built in. Rays are aimed only at the part of the
  rear element light gets through to each point of the sensor (the exit pupil), and a ray the lens
  blocks is cast again, so no ray is wasted on the aperture stop and the lens adds almost no grain;
  vignetting comes from the measured share of rays that get through.

  <img src="docs/images/lens.png" alt="scan_test through a pinhole, and through the double Gauss at f/2 focused on the glass sphere" width="650">
- **Color models:** the last step from pixel color to bytes is pluggable too: `clip` (the 1997
  look, and the default), `gamma`, `srgb`, `reinhard` tone mapping, and `hdr`.
- **Image output:** PNG (`--png`) and PPM (`--ppm`) beside the original raw format, and high
  dynamic range: `--hdr` writes every pixel's linear color, unclipped, as a Radiance HDR file
  that image editors, Blender and HDR viewers open. With the `hdr` color model, its `exposure`
  scales the colors first.
- **Readable scene files:** scenes are JSON that you can write and edit by hand, with clear errors
  that say where a mistake is.
- **Cross-platform scene editor:** `ray-editor`, built on wxWidgets for Windows, Linux and macOS:
  a scene tree, a property grid (and the JSON text) for every part of the scene, checked by the
  scene's own reader, a shaded OpenGL view with a wireframe overlay that you orbit, pan, zoom and
  click to select in, a *Render* button that ray-traces the view progressively over it, undo and
  redo, and STL and OBJ import. See [`editor/README.md`](editor/README.md).
- **Windows scene editor:** MFC MDI application with a scene tree, property dialogs for spheres, planes,
  point lights, the camera and the materials, and a material editor. Anything else, from any shape
  or light to the camera's sampler, color model and lens and the environment, can be edited, added
  or deleted as JSON from the tree's right-click menu, and *Import Model* brings in an STL or OBJ
  file.
- **Distributed rendering:** `rayengine` processes connect to the master over TCP and pull tiles of
  the image from its queue, trace them (camera rays, lens and sampling included) and send back the
  pixels; faster machines simply take more tiles. Engines can run anywhere that can reach the
  master, can join in the middle of a render, and if one drops out its unfinished tiles go to the
  others.
- **Local rendering:** the same master can trace with one thread per CPU instead, pulling tiles the
  same way.
- **Models:** meshes load STL and OBJ files directly; [`tools/obj_to_ray.py`](tools/obj_to_ray.py)
  can still turn an OBJ model into an inline mesh. The sample scenes include the Utah teapot and
  3DBenchy.

## How it works

```mermaid
flowchart LR
    subgraph Front["Front end"]
        UI["ray-editor or Ray.exe<br/>scene editors"]
        CLI["raythrower<br/>command line"]
    end
    subgraph Master["Task master"]
        TM["CTaskMaster::TraceScene<br/>(threads or TCP)"]
    end
    subgraph Tasks["Tracing tasks"]
        E1[rayengine]
        E2[rayengine]
        E3["worker thread"]
    end
    UI --> TM
    CLI --> TM
    TM -- "1. scene" --> E1 & E2 & E3
    E1 & E2 & E3 -- "2. request" --> TM
    TM -- "3. a tile" --> E1 & E2 & E3
    E1 & E2 & E3 -- "4. its pixels" --> TM
    TM -- pixels --> IMG[(Image file)]
```

1. The master splits the image into tiles (32×32 pixels; `--tile` changes it) and sends each task
   its own copy of the scene, as JSON.
2. Each task asks for work, and the master hands it the next tile.
3. The task traces the tile with `CTileTracer`: the camera's pixel sampler picks the rays, the
   camera (and its lens) makes them, and the sampler turns their colors into the tile's pixels,
   which go back to the master as linear colors. The task asks for another tile.
4. The master writes each tile's pixels through the camera's color model into the image. A task
   that goes away has its unfinished tiles handed to the others; when the image is done the master
   tells the tasks to stop.

Which primary rays are made for each pixel, and how their colors become the pixel, is up to the
camera's pixel sampler ([`core/include/PixelSampler.h`](core/include/PixelSampler.h)). The default
casts one ray through each pixel's center; `--aa corners` attaches the anti-aliasing sampler.
The color model ([`core/include/ColorModel.h`](core/include/ColorModel.h)) then turns each pixel's
linear color into bytes. Both are set in the scene's camera, or with `--aa` and `--color`. The
sampler runs in the tasks, one tile at a time; the color model runs on the master.

Over TCP every message is framed as a tag, a length, and a payload. The scene is sent as an SJSON
packet (IASLib's compact binary JSON) of the scene file's JSON. Requests, tiles and pixels are
small fixed-layout binary messages, the pixels as IEEE-754 doubles, so they cross exactly and
networked engines make the same image as threads; `RayWire::ToReadable` turns a tile or its pixels
into JSON for reading. See [`core/include/WireProtocol.h`](core/include/WireProtocol.h). An engine
opens the connection with a hello carrying the protocol version (4).

## Platforms

| Part | Linux | macOS | Windows |
|------|:-----:|:-----:|:-------:|
| `raythrower`, `rayengine`, core | ✅ GCC, Clang | ✅ Apple Clang | ✅ MSVC |
| `ray-editor` scene editor (wxWidgets) | ✅ GTK 3 | ✅ Cocoa | ✅ MSVC |
| `Ray.exe` scene editor (MFC) | | | ✅ MSVC with MFC |

All of them are built by CI on every push.

## Building

You need CMake 3.18 or later and a C++17 compiler. CMake downloads
[IASLib](https://github.com/jnaujok/IASLib) itself (pinned to the commit that added SJSON, until
IASLib tags a release with it), and nothing else is required. SJSON's optional LZ4 and zstd
support is left off, so every build is the same.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

This builds `raythrower` and `rayengine`. To build the cross-platform scene editor too, turn on
`RAY_BUILD_EDITOR`. CMake uses an installed wxWidgets if it finds one, and otherwise downloads
wxWidgets 3.2.9 and builds it (a few minutes, the first time). On Linux install the GTK 3 and
OpenGL development packages first (`sudo apt-get install libgtk-3-dev libgl1-mesa-dev`):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRAY_BUILD_EDITOR=ON
cmake --build build --target ray-editor --parallel
```

To build the Windows MFC scene editor, use Visual Studio with the *C++ MFC* component, which
includes the MBCS libraries the code needs:

```sh
cmake -S . -B build -DRAY_BUILD_GUI=ON
cmake --build build --config Release --target Ray
```

| Option | Default | What it does |
|--------|:-------:|--------------|
| `RAY_BUILD_EDITOR` | `OFF` | Build `ray-editor`, the cross-platform scene editor (wxWidgets) |
| `RAY_USE_SYSTEM_WXWIDGETS` | `ON` | Use an installed wxWidgets if CMake finds one, instead of downloading it |
| `RAY_BUILD_GUI` | `OFF` | Build the MFC scene editor (MSVC only) |
| `RAY_BUILD_TESTS` | `ON` | Build the regression tests |
| `FETCHCONTENT_SOURCE_DIR_IASLIB` | | Use a local IASLib checkout instead of downloading it |

## Usage

**Trace with threads on this machine:**

```sh
raythrower scenes/scan_test.ray -o scan_test.raw --png scan_test.png
```

**Keep the highlights** in a high dynamic range file, as well as a PNG:

```sh
raythrower scenes/scan_test.ray --color hdr:exposure=1.5 --png scan_test.png --hdr scan_test.hdr
```

**Smooth the edges** with five rays per pixel:

```sh
raythrower scenes/scan_test.ray --aa corners --ppm scan_test.ppm
```

**Tone map** instead of clipping highlights:

```sh
raythrower scenes/scan_test.ray --color reinhard --ppm scan_test.ppm
```

**Shoot through a real lens** (the camera's `lens` in the scene; see
[`scenes/lens_test.ray`](scenes/lens_test.ray)):

```sh
raythrower scenes/lens_test.ray --aa stratified:grid=8 --ppm lens.ppm
```

**Render the Utah teapot** (6,320 triangles):

```sh
raythrower scenes/teapot.ray --aa corners --ppm teapot.ppm
```

**Render the torture test**, every kind of object (the teapot too, in marble, and a layer-lined
3DBenchy, a colored-STL puzzle cube and an OBJ ring), mirrors, glass
(the cube absorbing by thickness, the ball frosted, a flint prism fanning colors into a spectrum), a die with drilled
pips (constructive solid geometry), textures (a wooden mirror frame), a brushed
gold tube, soft shadows from a sphere light, a rectangle light, a spot light and a low sun, and an
HDR sky in the mirrors and glass that lights the scene too, light bounced between surfaces, and a
light haze, through the double Gauss lens at 25 rays a pixel, each traced at four wavelengths
(spectral dispersion) and clamped at 1.5, at its full 1024×768 (about three and a half minutes on
an idle 16-core machine, with a Release build):

```sh
raythrower scenes/torture_test.ray --png torture.png --hdr torture.hdr
```

<img src="docs/images/torture_test.png" alt="The torture test at 1024x768: a marble teapot among mirrors, glass, a flint prism, a drilled die, an orange 3DBenchy, a puzzle cube, a textured ring, textured shapes and a wood-framed mirror wall under an HDR sky in a light haze, through a real lens" width="720">

**Bring in a model**: name an STL or OBJ file in a mesh, and place it (here a printable model,
which stands on z, turned to stand on y, 2 units across, on the floor):

```json
{ "type": "mesh", "file": "models/3DBenchy.stl", "fitSize": 2, "rotation": [-90, 0, 0], "align": "bottom",
  "materials": [ { "type": "diffuse", "color": [1, 1, 1], "texture": { "type": "layers" } } ] }
```

Or turn an OBJ model into an inline mesh (`--weld` joins repeated vertices so the surface is smooth
across seams):

```sh
tools/obj_to_ray.py model.obj --smooth --weld > model_mesh.json
```

**Trace with engines on this machine**, started for you:

```sh
raythrower scenes/scan_test.ray --listen --spawn -N8
```

**Trace across machines.** Start the master, then an engine on each machine:

```sh
raythrower scenes/scan_test.ray --listen 7997 -N3      # on the master
rayengine master.example.com 7997                       # on each of three machines
```

| `raythrower` option | Meaning |
|--------|---------|
| `-N<count>`, `--tasks <count>` | Number of tracing tasks (default: one per CPU) |
| `-o`, `--output <file>` | Image file (default: the one named in the scene) |
| `--ppm <file>` | Also write the image as a PPM file |
| `--png <file>` | Also write the image as a PNG file |
| `--hdr <file>` | Also write the linear, unclipped colors as a Radiance HDR (`.hdr`) file, for high dynamic range |
| `--listen [port]` | Use engines over TCP on this port (default 7997; 0 picks a free one) |
| `--spawn` | Start the engines on this machine |
| `--engine <path>` | The `rayengine` to start (default: the one next to `raythrower`) |
| `--timeout <seconds>` | How long to wait for engines to connect (default 60) |
| `--tile <pixels>` | Size of the square tiles tasks ask for (default 32) |
| `--aa <sampler>` | Pixel sampler, instead of the scene's: `none`, `corners`, `box`, `random`, `stratified`, or `adaptive`; settings after a colon, as in `random:samples=9` or `adaptive:min=16,threshold=0.005` |
| `--color <model>` | Color model, instead of the scene's: `clip`, `gamma`, `srgb`, `reinhard`, or `hdr`; settings as for `--aa`, as in `reinhard:exposure=1.5` |
| `--size <w>x<h>` | Image size, instead of the scene's |
| `--save <file>` | Save the scene (with any `--aa`/`--color`/`--size`) as a scene archive, textures inside, and exit |
| `--save-json <file>` | Save it as plain JSON, with the files it uses beside it, and exit |

Images are written in the original raw format: a 2-byte width, a 2-byte height (both big-endian),
then RGB bytes. `--png` and `--ppm` give copies any image viewer can open. `--hdr` keeps what the
8-bit formats lose: colors brighter than white, at floating-point precision (Radiance RGBE: a byte
each for red, green and blue and a shared exponent). It holds the color model's linear colors:
the traced colors as they are, or scaled by the `hdr` or `reinhard` model's `exposure`.

**Cross-platform scene editor:** run `ray-editor` (or `ray-editor scenes/torture_test.ray`). The
tree on the left lists the settings, camera, environment, objects (with their materials) and
lights; the panel on the right edits the selected one as a property grid or as JSON; the view in
the middle shows the scene shaded with OpenGL. Drag to orbit, right-drag to pan, use the wheel to
zoom and click an object to select it. *View > Wireframe Overlay* draws the triangles' edges,
*View > Look Through Scene Camera* shows what the camera sees, and *Render > Render View* (F5)
ray-traces the view over it, a quick pass first. *Edit* adds objects, lights and a sky, and
*File > Import Model* brings in an STL or OBJ file. See [`editor/README.md`](editor/README.md).

<img src="docs/images/editor.png" alt="ray-editor on Linux: the scene tree on the left, the textured teapot ray-traced in the view after Render, and the settings in the property grid on the right" width="720">

**Windows scene editor:** run `Ray.exe`, open a scene from [`scenes/`](scenes) or build one with the *Add*
menu, and choose *Render*. Right-click the scene's name in the tree to edit its settings, camera or
environment as JSON, add any object or light as JSON, or import a model; right-click an object or
light to edit it as JSON or delete it.

## Testing

```sh
ctest --test-dir build --output-on-failure
```

The torture test, the Benchy close-up and the prism (and their network twins) are labelled `slow`;
`ctest --test-dir build -LE slow` runs everything else in a minute or two. CI runs everything, on
every system.

The tests render every sample scene and compare it with a reference render (four of them again with
corner anti-aliasing, one with each other sampler and color model, the teapot, two real lenses,
textures, solid textures and absorbing glass, rough glass and mirrors with area lights, bump and normal maps with PNG textures, spot and
directional lights, an HDR environment map, cylinders, cones, and boxes, constructive solid geometry, a dispersive prism, models from colored STL and OBJ files,
3DBenchy with printed layer lines, light bounced between surfaces, haze and ground mist, and a torture test of all of them through a real lens).
They also check the scene editor's logic without a display (`editor_logic`): editing parts as
JSON with undo and redo, the new-object templates, tessellation, that the view aims rays exactly as
the tracer's camera does, picking, model import, saving, and the progressive render.
They check that a lens has the focal length, f-number, focus and color dispersion its
prescription should, that
networked engines produce exactly the same bytes as threads (even when an engine drops out mid-render
and another joins), and that tiles and pixels cross the wire
exactly (including texture images embedded in the scene), that scene files survive a load and save, that mistakes in JSON scenes are reported where
they are, triangle sidedness and that smooth mesh faces agree along shared edges, where rays hit
cylinders, cones, and boxes and the normals there, and that a mesh's bounding volume hierarchy finds
exactly the hits testing every face does (on the teapot and on 3DBenchy), and that the scene's tree finds exactly the hits and
shadows testing every object does. They check that gradient noise is smooth, bounded and set by its
seed, and that glass absorbs exactly as Beer–Lambert says for the distance through it. They check
that rough surfaces draw their microfacets from the GGX distribution, that rough glass is the same
every time for the same ray, that a sphere light's points lie on the disc it shows, that a
rectangle light shines only to its front, and that hiding half of one gives half its light. They
check that PNG images of every color type and bit depth, interlaced or not, decode exactly, that
damaged or truncated ones are refused, and that height and normal maps tilt the normal the way
and by the amount they should. They check that a spot light's cone fades along a smoothstep, that
sunlight is the same everywhere and casts shadows the size of what casts them (soft with an angle),
that HDR images decode exactly, flat or run-length encoded, that an environment shows each pixel
along its own direction and wraps around, and that the sky's light is right for a uniform sky, a
half sky and the foot of a wall. They check where rays hit a union, an intersection, a difference
and a nested csg and the normals there (inside out on a subtracted shape), from outside, inside and
leaving the surface, that each surface takes its own shape's materials with `childMaterials`, that
dispersive glass bends each color by the index a lens's glass would have, that a ray in every color
splits into exactly what rays traced in each color alone see, that spectral weights add up to
white over the visible range, that a wavelength bends as its line's index says and is picked the
same way every time, that a camera without dispersive glass sees the same in spectral mode, that
every thread's copy of a scene shares a mesh's geometry and lets it go with the last one, and that
the sample clamp caps each channel. With Python 3 installed, they also decode the PNG and
HDR output (with Python's own zlib) and check it against the raw image. CI runs them
on every platform, and again on Linux under AddressSanitizer and UndefinedBehaviorSanitizer.

### Expected output

These are the reference renders the tests compare against, from
[`tests/reference/renders.tar.gz`](tests/reference/renders.tar.gz), each labelled with its test.
They are shown at up to 320 pixels wide; `tools/render_previews.py` makes these previews from the
archive, so run it whenever the references are regenerated. The networked tests (`network_*`) must
produce exactly the same images as the threaded ones, so they have no images of their own.

**Sample scenes**, with each scene's own sampler and color model (one ray a pixel, `clip`).
`huh` and `ser_test2` have nothing in view. `triangle2`'s corners are given tilted normals that fake
a curve; they are what light it, since its lights lie in the triangle's own plane.

<table>
<tr><td align="center"><img src="docs/images/tests/axis_test.png" alt="render_axis_test" width="260"><br><code>render_axis_test</code></td><td align="center"><img src="docs/images/tests/huh.png" alt="render_huh (blank by design)" width="260"><br><code>render_huh (blank by design)</code></td><td align="center"><img src="docs/images/tests/inside_sphere.png" alt="render_inside_sphere" width="260"><br><code>render_inside_sphere</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/mat_test.png" alt="render_mat_test" width="260"><br><code>render_mat_test</code></td><td align="center"><img src="docs/images/tests/reflect_me.png" alt="render_reflect_me" width="260"><br><code>render_reflect_me</code></td><td align="center"><img src="docs/images/tests/reflect_test2.png" alt="render_reflect_test2" width="260"><br><code>render_reflect_test2</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/reflect_test3.png" alt="render_reflect_test3" width="260"><br><code>render_reflect_test3</code></td><td align="center"><img src="docs/images/tests/reflection_test.png" alt="render_reflection_test" width="260"><br><code>render_reflection_test</code></td><td align="center"><img src="docs/images/tests/scan_test.png" alt="render_scan_test" width="260"><br><code>render_scan_test</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/ser_test.png" alt="render_ser_test" width="260"><br><code>render_ser_test</code></td><td align="center"><img src="docs/images/tests/ser_test2.png" alt="render_ser_test2 (blank by design)" width="260"><br><code>render_ser_test2 (blank by design)</code></td><td align="center"><img src="docs/images/tests/spec_test.png" alt="render_spec_test" width="260"><br><code>render_spec_test</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/test.png" alt="render_test" width="260"><br><code>render_test</code></td><td align="center"><img src="docs/images/tests/triangle.png" alt="render_triangle" width="260"><br><code>render_triangle</code></td><td align="center"><img src="docs/images/tests/triangle2.png" alt="render_triangle2" width="160"><br><code>render_triangle2</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/xxx.png" alt="render_xxx" width="260"><br><code>render_xxx</code></td><td align="center"><img src="docs/images/tests/teapot.png" alt="render_teapot (160x120)" width="160"><br><code>render_teapot (160x120)</code></td><td align="center"><img src="docs/images/tests/lens_test.png" alt="render_lens (160x120)" width="160"><br><code>render_lens (160x120)</code></td><td align="center"><img src="docs/images/tests/chromatic_test.png" alt="render_chromatic (160x120)" width="160"><br><code>render_chromatic (160x120)</code></td><td align="center"><img src="docs/images/tests/texture_test.png" alt="render_texture" width="260"><br><code>render_texture</code></td><td align="center"><img src="docs/images/tests/teapot_textured.png" alt="render_teapot_textured (120x90)" width="120"><br><code>render_teapot_textured (120x90)</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/primitives_test.png" alt="render_primitives" width="260"><br><code>render_primitives</code></td><td align="center"><img src="docs/images/tests/torture_test.png" alt="render_torture (160x120)" width="160"><br><code>render_torture (160x120)</code></td><td align="center"><img src="docs/images/tests/solid_textures_test.png" alt="render_solid_textures" width="260"><br><code>render_solid_textures</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/soft_test.png" alt="render_soft" width="260"><br><code>render_soft</code></td><td align="center"><img src="docs/images/tests/bump_test.png" alt="render_bump" width="260"><br><code>render_bump</code></td><td align="center"><img src="docs/images/tests/lights_test.png" alt="render_lights" width="260"><br><code>render_lights</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/environment_test.png" alt="render_environment" width="260"><br><code>render_environment</code></td><td align="center"><img src="docs/images/tests/csg_test.png" alt="render_csg" width="260"><br><code>render_csg</code></td><td align="center"><img src="docs/images/tests/prism_test.png" alt="render_prism" width="260"><br><code>render_prism</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/models_test.png" alt="render_models" width="260"><br><code>render_models</code></td><td align="center"><img src="docs/images/tests/benchy_test.png" alt="render_benchy" width="260"><br><code>render_benchy</code></td><td align="center"><img src="docs/images/tests/bounce_test.png" alt="render_bounce (160x120)" width="160"><br><code>render_bounce (160x120)</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/haze_test.png" alt="render_haze (160x120)" width="160"><br><code>render_haze (160x120)</code></td></tr>
</table>

**Samplers** (`--aa`): corner anti-aliasing on four scenes, and the `box`, `random`,
`stratified` (3×3) and `adaptive` samplers on `scan_test` (compare with `render_scan_test` above).

<table>
<tr><td align="center"><img src="docs/images/tests/scan_test_aa.png" alt="render_aa_scan_test (corners)" width="260"><br><code>render_aa_scan_test (corners)</code></td><td align="center"><img src="docs/images/tests/axis_test_aa.png" alt="render_aa_axis_test (corners)" width="260"><br><code>render_aa_axis_test (corners)</code></td><td align="center"><img src="docs/images/tests/triangle_aa.png" alt="render_aa_triangle (corners)" width="260"><br><code>render_aa_triangle (corners)</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/xxx_aa.png" alt="render_aa_xxx (corners)" width="260"><br><code>render_aa_xxx (corners)</code></td><td align="center"><img src="docs/images/tests/scan_test_box.png" alt="render_sampler_box" width="260"><br><code>render_sampler_box</code></td><td align="center"><img src="docs/images/tests/scan_test_random2.png" alt="render_sampler_random2" width="260"><br><code>render_sampler_random2</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/scan_test_random4.png" alt="render_sampler_random4" width="260"><br><code>render_sampler_random4</code></td><td align="center"><img src="docs/images/tests/scan_test_random9.png" alt="render_sampler_random9" width="260"><br><code>render_sampler_random9</code></td><td align="center"><img src="docs/images/tests/scan_test_stratified.png" alt="render_sampler_stratified" width="260"><br><code>render_sampler_stratified</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/scan_test_adaptive.png" alt="render_sampler_adaptive" width="260"><br><code>render_sampler_adaptive</code></td></tr>
</table>

**Color models** (`--color`) on `scan_test`. The 1997 scenes were lit for screens without gamma
encoding, so `gamma` and `srgb` look washed out; `reinhard` brings back clipped highlights. `hdr`
at its default exposure looks like `srgb` here: what it adds is the `--hdr` file.

<table>
<tr><td align="center"><img src="docs/images/tests/scan_test.png" alt="clip (the default)" width="260"><br><code>clip (the default)</code></td><td align="center"><img src="docs/images/tests/scan_test_gamma.png" alt="render_color_gamma" width="260"><br><code>render_color_gamma</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/scan_test_srgb.png" alt="render_color_srgb" width="260"><br><code>render_color_srgb</code></td><td align="center"><img src="docs/images/tests/scan_test_reinhard.png" alt="render_color_reinhard" width="260"><br><code>render_color_reinhard</code></td></tr>
<tr><td align="center"><img src="docs/images/tests/scan_test_hdr.png" alt="render_color_hdr" width="260"><br><code>render_color_hdr</code></td></tr>
</table>

## Repository layout

| Path | Contents |
|------|----------|
| [`core/`](core) | The ray tracer: scene, objects, materials, serialization, task masters, TCP transport |
| [`raythrower/`](raythrower) | Command-line master |
| [`rayengine/`](rayengine) | Networked tracing task |
| [`editor/`](editor) | `ray-editor`, the cross-platform scene editor: `logic/` (no toolkit: the document, tessellation, the view's camera, the progressive render) and `gui/` (wxWidgets) |
| [`src/`](src), [`include/`](include) | MFC scene editor |
| [`res/`](res) | Resource scripts, icons, and bitmaps for the scene editor |
| [`help/`](help) | WinHelp project for the scene editor |
| [`scenes/`](scenes) | Sample `.ray` scenes: the 1997 test scenes, as JSON, and newer ones (the Utah teapot, lenses, textures, models); `scenes/textures/` holds texture images and `scenes/models/` model files, among them 3DBenchy (CC0, see [NOTICE](NOTICE)) |
| [`tools/`](tools) | `obj_to_ray.py`, which turns OBJ models into meshes; `render_previews.py`, which makes the README's expected-output images; `check_image.py`, which checks PNG and HDR output; `make_textures.py`, which makes the brick textures and the sky; `make_models.py`, which makes the small model files |
| [`tests/`](tests) | Regression tests and reference renders |
| [`cmake/`](cmake) | The scene editors' builds: `RayEditor.cmake` (wxWidgets) and `RayGui.cmake` (MFC) |
| [`docs/`](docs) | MFC AppWizard notes, README images, and the changelog's before and after images |
| [`legacy/`](legacy) | Visual C++ 4, 5, and 6 project files, kept for history |

## Scene files

A saved `.ray` file is an [IASLib](https://github.com/jnaujok/IASLib) resource archive holding the
scene and every file it uses, such as texture images, so one file carries the whole scene. The
scene inside is JSON: render settings, the camera with its sampler and color model, objects with
their materials, and lights. Plain JSON `.ray` files load too (the sample scenes are kept that way),
and `raythrower --save-json` writes one, with the files it uses beside it, for editing by hand.
Only the camera is required; anything else left out takes its default.

```json
{
    "format": "Ray scene",
    "version": 3,
    "settings": { "imageWidth": 640, "imageHeight": 480, "imageFile": "ball.out" },
    "camera": {
        "position": [0, 1, -10],
        "lookAt": [0, 0, 0],
        "verticalAngle": 30,
        "sampler": "corners",
        "colorModel": { "type": "reinhard", "exposure": 1.5 }
    },
    "objects": [
        { "type": "plane", "position": [0, -1, 0], "normal": [0, 1, 0] },
        {
            "type": "sphere", "center": [0, 0, 0], "radius": 1,
            "materials": [
                { "type": "diffuse", "influence": 0.6, "color": [0.8, 0.2, 0.2] },
                { "type": "reflective", "influence": 0.4, "color": [1, 1, 1] }
            ]
        }
    ],
    "lights": [ { "type": "point", "position": [5, 8, -6], "color": [1, 1, 1] } ]
}
```

Numbers are written rounded to six decimal places. The binary files of 1996–1999 are no longer
read; the sample scenes were converted before that format was dropped.
[`scenes/README.md`](scenes/README.md) describes every field, lists the sample scenes, and gives
the format's history.

## Project status

The code builds and runs on current compilers, and work on the ray tracing has resumed: a corrected
camera, anti-aliasing, color models, meshes (with a bounding volume hierarchy), cylinders, cones
and boxes, solid marble, wood and stone textures, glass that absorbs by thickness, frosted glass
and brushed mirrors, area lights with soft shadows, bump and normal maps, PNG textures, spot and
directional lights, environment maps with image-based lighting, constructive solid geometry,
dispersive glass with spectral dispersion, STL and OBJ model files with printed layer lines and
lean, shared meshes, light bounced between surfaces, haze, and JSON scenes so far, and a cross-platform scene editor with a shaded
OpenGL view and progressive rendering. Both editors reach every feature through JSON editing; next
up is light bouncing between surfaces and atmospheric haze.

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## Contributing

This repository is **not accepting outside contributions**. Please don't open pull requests.
See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Licensed under the [Apache License, Version 2.0](LICENSE). See [NOTICE](NOTICE) for attribution.
The build uses [IASLib](https://github.com/jnaujok/IASLib), which is licensed under the LGPL-3.0.

Copyright © 1997–2026 Jeffrey Naujok.
