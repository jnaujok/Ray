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
   (version 3: the scene as SJSON, batches as fixed-layout binary).
2. Receives the scene, as SJSON.
3. Receives batches of primary rays, traces each one, and sends the colors back.
4. Exits when it is sent an empty batch, or when the master goes away.

The engine is a `CSocketTaskSlave` (`core/src/SocketTaskSlave.cpp`) running the shared
`CTaskSlave::RunEngine` loop; the code is in [`main.cpp`](main.cpp). It replaces the 1997 PVM
worker, which `pvm_spawn` started; `raythrower --spawn` now starts local engines instead.

## Building

Built with the rest of the project from the repository root:

```sh
cmake -S . -B build && cmake --build build --target rayengine
```
