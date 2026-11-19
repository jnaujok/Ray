# raythrower

[![Platform: Unix](https://img.shields.io/badge/platform-Unix-lightgrey.svg)](#)
[![PVM 3](https://img.shields.io/badge/PVM-3-orange.svg)](https://www.netlib.org/pvm3/)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](../LICENSE)

A command-line PVM master for the [Ray](../README.md) distributed ray tracer. It traces a
`.ray` scene on a PVM cluster without the Windows scene editor.

## Usage

```sh
raythrower <FileToTrace> [-N<tasks>]
```

| Argument | Meaning |
|----------|---------|
| `FileToTrace` | Full path of the `.ray` scene to trace |
| `-N<tasks>` | Number of [`rayengine`](../rayengine) tasks to spawn under PVM |

The thrower loads the scene, spawns the engines, multicasts the scene to them, deals out
batches of rays, and writes the returned pixels to the image file named in the scene's settings.
When it finishes, it prints the pixel count, elapsed time, and pixels per second.

It must be run as a PVM master, not as a spawned task.

## Layout

| Path | Contents |
|------|----------|
| `src/main_unix.cpp` | Unix entry point (used by the Makefile and CMake) |
| `src/main.cpp` | Win32 variant of the same entry point |
| `src/CString.cpp`, `include/CString.h` | Standalone `CString` replacement for non-MFC builds |
| `Makefile` | Make build. It shares the engine sources in `../rayengine/src` and the application headers in `../include` |

## Building

```sh
make PVM_INCLUDE=/path/to/pvm3/include PVM_LIB=/path/to/pvm3/lib/ARCH
```

Or from the repository root: `cmake -S . -B build -DRAY_BUILD_PVM_TOOLS=ON`.
