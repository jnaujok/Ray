# raythrower

[![Build and test](https://github.com/jnaujok/Ray/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/jnaujok/Ray/actions/workflows/build.yml)
[![Platforms](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](#)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](../LICENSE)

The command-line master of the [Ray](../README.md) distributed ray tracer. It traces a `.ray`
scene without the Windows scene editor, with threads or with networked
[`rayengine`](../rayengine) processes.

## Usage

```sh
raythrower <FileToTrace> [options]
```

| Option | Meaning |
|--------|---------|
| `-N<count>`, `--tasks <count>` | Number of tracing tasks (default: one per CPU) |
| `-o`, `--output <file>` | Image file to write (default: the one named in the scene) |
| `--ppm <file>` | Also write the image as a PPM file |
| `--listen [port]` | Trace with engines that connect on this port (default 7997; 0 picks a free port) |
| `--spawn` | With `--listen`, start the engines on this machine |
| `--engine <path>` | The `rayengine` to start (default: next to `raythrower`) |
| `--timeout <seconds>` | How long to wait for each engine to connect (default 60) |
| `--batch <rays>` | Rays per batch (default 127) |
| `--aa <sampler>` | Pixel sampler, instead of the scene's: `none`, `corners`, `box`, `random`, or `stratified`, with any settings after a colon (`random:samples=9,seed=2`, `stratified:grid=8`) |
| `--color <model>` | Color model, instead of the scene's: `clip`, `gamma`, `srgb`, or `reinhard`, with settings as for `--aa` (`reinhard:exposure=1.5`) |
| `--size <w>x<h>` | Image size, instead of the scene's |
| `--save <file>` | Save the scene, with any `--aa`, `--color` or `--size` applied, and exit without tracing |
| `-q`, `--quiet` | Print only errors |

Without `--listen` it traces with one thread per task. When the trace finishes it prints the pixel
count, the rays traced, the elapsed time, and the pixels per second.

With `--aa corners` each pixel is the average of five rays: one at each corner and one at the
center. A corner is traced once and shared with the neighboring pixels, so this costs about two rays
per pixel. The engines don't need to know: the camera's sampler (`CCornerSampler`) labels the rays with
their position on a doubled sample grid and does the averaging in the master.

`--aa box` traces four rays per pixel, each halfway from the pixel's center to one of its corners,
and averages them equally.

`--aa random:samples=N` traces one ray through the center, which counts for half the pixel, and N
rays at random points in the pixel, which share the other half: N + 1 rays a pixel (N from 1 to
1024, default 4). The points come from a hash of the pixel, the ray's number and `seed` (default 0),
so the same scene always gives the same image, however the work is split between tasks.

`--aa stratified:grid=n` cuts each pixel into an n×n grid and traces one ray at a random point in
each cell, all weighted equally (n from 1 to 32, default 4). It is the sampler to use with a real
lens, where every ray takes its own path through the glass.

When the scene's camera has a lens, `raythrower` prints its focal length, f-number, focus distance
and where the sensor sits before it traces, and, for a lens with chromatic aberration, its focal
length in red, green and blue. Such a lens traces three rays per sample.

`--color` picks how pixel colors become bytes. `clip`, the default, is the 1997 look. `gamma` and
`srgb` encode for today's displays, which makes the 1997 scenes, lit for a screen without gamma
encoding, look washed out. `reinhard` tone maps, bringing back highlights that `clip` turns white.
Settings such as a Reinhard exposure are given in the scene file (see
[`scenes/README.md`](../scenes/README.md)). `--help` lists every sampler and color model.

## Examples

```sh
# Threads on this machine
raythrower scenes/scan_test.ray -o scan.raw --ppm scan.ppm

# The same, anti-aliased and tone mapped
raythrower scenes/scan_test.ray --aa corners --color reinhard -o scan.raw --ppm scan.ppm

# Save a copy of a scene with anti-aliasing turned on in it
raythrower scenes/scan_test.ray --aa corners --save scan_test_aa.ray

# Four local engines over TCP
raythrower scenes/scan_test.ray --listen --spawn -N4

# Engines on other machines: start this, then `rayengine <this-host>` on each
raythrower scenes/scan_test.ray --listen 7997 -N3
```

## How it works

`raythrower` loads the scene with `CScene::Load` and traces it with the shared `CTaskMaster::TraceScene` loop through a `CThreadTaskMaster` or a
`CSocketTaskMaster`. The code is in [`main.cpp`](main.cpp).

## Building

Built with the rest of the project from the repository root:

```sh
cmake -S . -B build && cmake --build build --target raythrower
```
