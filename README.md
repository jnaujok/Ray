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
- [Repository layout](#repository-layout)
- [Scene files](#scene-files)
- [Project status](#project-status)
- [Changelog](#changelog)
- [Contributing](#contributing)
- [License](#license)

## About

Ray is a ray tracer written in 1996–1997 that splits rendering across a network of machines.
You build and edit scenes in a Windows MFC application, and the image is traced in parallel by
engine processes on any number of machines, or by worker threads on one.

It was originally written for [PVM](https://www.netlib.org/pvm3/) (Parallel Virtual Machine) on
a cluster of Unix workstations. The cross-platform library written for it grew into
[IASLib](https://github.com/jnaujok/IASLib), and the ray tracer now runs on IASLib 3: PVM is
replaced by plain TCP, and the command-line programs build on Windows, Linux, and macOS.
The ray tracing itself is the 1997 code.

<div align="center">
<img src="docs/images/gallery.png" alt="Anti-aliased renders of six of the sample scenes, including the Utah teapot" width="720">
</div>

## Features

- **Primitives:** spheres, planes, triangles, and triangle meshes. A triangle's normal comes from
  its corner order; it can be single- or double-sided, and flat or smooth. A mesh shares vertices
  between faces, gives each vertex the sum of its faces' normals, and skips all its faces for rays
  that miss its bounding box.
- **Lights:** point lights.
- **Materials:** ambient, diffuse, specular, reflective, and transparent, mixed per object by influence.
- **Anti-aliasing:** the camera's primary rays come from a pluggable pixel sampler. The first
  anti-aliasing sampler takes five rays per pixel (four shared corners and the center) at a cost
  of about two.
- **Camera:** pinhole camera with position, look-at point, up vector, and vertical field of view;
  square pixels at any image size.
- **Color models:** the last step from pixel color to bytes is pluggable too: `clip` (the 1997
  look, and the default), `gamma`, `srgb`, and `reinhard` tone mapping.
- **Readable scene files:** scenes are JSON that you can write and edit by hand, with clear errors
  that say where a mistake is.
- **Scene editor:** MFC MDI application with a scene tree, property dialogs for every object type,
  and a material editor.
- **Distributed rendering:** a master hands out batches of rays to `rayengine` processes over TCP and
  collects the traced colors. Engines can run anywhere that can reach the master.
- **Local rendering:** the same master can trace with one thread per CPU instead.
- **Models:** [`tools/obj_to_ray.py`](tools/obj_to_ray.py) turns Wavefront OBJ models into meshes;
  the sample scenes include the Utah teapot.

## How it works

```mermaid
flowchart LR
    subgraph Front["Front end"]
        UI["Ray.exe<br/>MFC scene editor"]
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
    TM -- "2. batches of rays" --> E1 & E2 & E3
    E1 & E2 & E3 -- "3. traced colors" --> TM
    TM -- pixels --> IMG[(Image file)]
```

1. The master sends each task its own copy of the scene, as JSON.
2. It walks the image, builds batches of primary rays (`CRayArray`), and deals them out in turn.
3. Each task traces its batch and sends back the colors. The camera's pixel sampler turns them
   into pixels, and its color model turns the pixels into the image's bytes.
4. An empty batch tells a task to stop.

Which primary rays are made for each pixel, and how their colors become the pixel, is up to the
camera's pixel sampler ([`core/include/PixelSampler.h`](core/include/PixelSampler.h)). The default
casts one ray through each pixel's center; `--aa corners` attaches the anti-aliasing sampler.
The color model ([`core/include/ColorModel.h`](core/include/ColorModel.h)) then turns each pixel's
linear color into bytes. Both are set in the scene's camera, or with `--aa` and `--color`, and the
engines never need to know which are in use.

Over TCP every message is framed as a tag, a length, and a payload. The scene is sent as an SJSON
packet (IASLib's compact binary JSON) of the scene file's JSON. The batches of rays and colors,
which are sent constantly, are fixed-layout IEEE-754 doubles, because SJSON costs far more per ray
to encode than tracing the ray does; `RayWire::ToReadable` turns a batch into JSON for reading.
Numbers cross exactly either way. See [`core/include/WireProtocol.h`](core/include/WireProtocol.h).
An engine opens the connection with a hello carrying the protocol version (3).

## Platforms

| Part | Linux | macOS | Windows |
|------|:-----:|:-----:|:-------:|
| `raythrower`, `rayengine`, core | ✅ GCC, Clang | ✅ Apple Clang | ✅ MSVC |
| `Ray.exe` scene editor | | | ✅ MSVC with MFC |

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

This builds `raythrower` and `rayengine`. To build the scene editor, use Visual Studio with the
*C++ MFC* component, which includes the MBCS libraries the code needs:

```sh
cmake -S . -B build -DRAY_BUILD_GUI=ON
cmake --build build --config Release --target Ray
```

| Option | Default | What it does |
|--------|:-------:|--------------|
| `RAY_BUILD_GUI` | `OFF` | Build the MFC scene editor (MSVC only) |
| `RAY_BUILD_TESTS` | `ON` | Build the regression tests |
| `FETCHCONTENT_SOURCE_DIR_IASLIB` | | Use a local IASLib checkout instead of downloading it |

## Usage

**Trace with threads on this machine:**

```sh
raythrower scenes/scan_test.ray -o scan_test.raw --ppm scan_test.ppm
```

**Smooth the edges** with five rays per pixel:

```sh
raythrower scenes/scan_test.ray --aa corners --ppm scan_test.ppm
```

**Tone map** instead of clipping highlights:

```sh
raythrower scenes/scan_test.ray --color reinhard --ppm scan_test.ppm
```

**Render the Utah teapot** (6,320 triangles; every ray inside its bounding box tests every face,
so it takes a while):

```sh
raythrower scenes/teapot.ray --aa corners --ppm teapot.ppm
```

**Bring in an OBJ model** as a mesh (`--weld` joins repeated vertices so the surface is smooth
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
| `--listen [port]` | Use engines over TCP on this port (default 7997; 0 picks a free one) |
| `--spawn` | Start the engines on this machine |
| `--engine <path>` | The `rayengine` to start (default: the one next to `raythrower`) |
| `--timeout <seconds>` | How long to wait for engines to connect (default 60) |
| `--batch <rays>` | Rays per batch (default 127) |
| `--aa <mode>` | Pixel sampler, instead of the scene's: `none` or `corners` |
| `--color <model>` | Color model, instead of the scene's: `clip`, `gamma`, `srgb`, or `reinhard` |
| `--size <w>x<h>` | Image size, instead of the scene's |
| `--save <file>` | Save the scene (with any `--aa`/`--color`/`--size`) and exit |

Images are written in the original raw format: a 2-byte width, a 2-byte height (both big-endian),
then RGB bytes. `--ppm` gives a copy any image viewer can open.

**Scene editor:** run `Ray.exe`, open a scene from [`scenes/`](scenes) or build one with the *Add*
menu, and choose *Render*.

## Testing

```sh
ctest --test-dir build --output-on-failure
```

The tests render every sample scene and compare it with a reference render (four of them again
with anti-aliasing, one with each color model, and the teapot), check that networked engines
produce exactly the same bytes as threads and that rays and colors cross the wire exactly, check
that scene files survive a load and save, check that mistakes in JSON scenes are reported where
they are, and check triangle sidedness and that smooth mesh faces agree along shared edges. CI runs them on every platform, and again on Linux under
AddressSanitizer and UndefinedBehaviorSanitizer.

## Repository layout

| Path | Contents |
|------|----------|
| [`core/`](core) | The ray tracer: scene, objects, materials, serialization, task masters, TCP transport |
| [`raythrower/`](raythrower) | Command-line master |
| [`rayengine/`](rayengine) | Networked tracing task |
| [`src/`](src), [`include/`](include) | MFC scene editor |
| [`res/`](res) | Resource scripts, icons, and bitmaps for the scene editor |
| [`help/`](help) | WinHelp project for the scene editor |
| [`scenes/`](scenes) | Sample `.ray` scenes: the 1997 test scenes, as JSON, and the Utah teapot |
| [`tools/`](tools) | `obj_to_ray.py`, which turns OBJ models into meshes |
| [`tests/`](tests) | Regression tests and reference renders |
| [`cmake/`](cmake) | The scene editor's build |
| [`docs/`](docs) | MFC AppWizard notes and README images |
| [`legacy/`](legacy) | Visual C++ 4, 5, and 6 project files, kept for history |

## Scene files

`.ray` files are JSON: render settings, the camera with its sampler and color model, objects with
their materials, and lights. Only the camera is required; anything else left out takes its default.

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
camera, anti-aliasing, color models, meshes, and JSON scenes so far. Next up are faster mesh
intersection and choices for sampling and color in the scene editor.

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## Contributing

This repository is **not accepting outside contributions**. Please don't open pull requests.
See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Licensed under the [Apache License, Version 2.0](LICENSE). See [NOTICE](NOTICE) for attribution.
The build uses [IASLib](https://github.com/jnaujok/IASLib), which is licensed under the LGPL-3.0.

Copyright © 1997–2026 Jeffrey Naujok.
