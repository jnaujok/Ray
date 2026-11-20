# rayengine

[![Build and test](https://github.com/jnaujok/Ray/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/jnaujok/Ray/actions/workflows/build.yml)
[![Platforms](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](#)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](../LICENSE)

The tracing task of the [Ray](../README.md) distributed ray tracer.

## Usage

```sh
rayengine <master-host> [port] [--retry <seconds>]
```

| Argument | Meaning |
|----------|---------|
| `master-host` | Host name or address of a `raythrower` started with `--listen` |
| `port` | The master's port (default 7997) |
| `--retry <seconds>` | Keep trying to connect for this long if the master isn't listening yet |

## What it does

1. Connects to the master and sends a hello with the protocol version. The master turns away an
   engine of a different version, so engines and masters must come from the same release
   (version 4: the scene as SJSON, then tiles of the image).
2. Receives the scene, as SJSON, and asks for a tile.
3. Traces each tile it is given (making its own camera rays, through the lens if the camera has
   one, and sampling them as the camera says), sends the tile's pixels back, and asks for the next.
4. Exits when the master tells it to stop, or goes away. An engine can connect at any time, even
   in the middle of a render: it gets the scene and starts asking for tiles.

The engine is a `CSocketTaskSlave` (`core/src/SocketTaskSlave.cpp`) running the shared
`CTaskSlave::RunEngine` loop; the code is in [`main.cpp`](main.cpp). It replaces the 1997 PVM
worker, which `pvm_spawn` started; `raythrower --spawn` now starts local engines instead.

## Building

Built with the rest of the project from the repository root:

```sh
cmake -S . -B build && cmake --build build --target rayengine
```
