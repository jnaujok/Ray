# rayengine

[![Platform: Unix](https://img.shields.io/badge/platform-Unix-lightgrey.svg)](#)
[![PVM 3](https://img.shields.io/badge/PVM-3-orange.svg)](https://www.netlib.org/pvm3/)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](../LICENSE)

The PVM worker process of the [Ray](../README.md) distributed ray tracer.

## What it does

`rayengine` is started by a master (the Windows scene editor or [`raythrower`](../raythrower))
through `pvm_spawn`. It:

1. receives the serialized scene (`MESSAGE_SCENE`),
2. receives a batch of primary rays (`MESSAGE_RAYS`),
3. traces each ray and sends the resulting colors back to its parent, and
4. repeats until it receives an empty batch, then exits.

It refuses to run outside a PVM virtual machine.

## Layout

| Path | Contents |
|------|----------|
| `src/` | Engine sources: geometry, materials, scene, streams, `main.cpp` |
| `include/` | Engine headers (Unix copies of the shared ray tracing core) |
| `Makefile` | Make build |

## Building

```sh
make PVM_INCLUDE=/path/to/pvm3/include PVM_LIB=/path/to/pvm3/lib/ARCH
```

Or from the repository root: `cmake -S . -B build -DRAY_BUILD_PVM_TOOLS=ON`.

> [!NOTE]
> `src/PVMStream.cpp` was never in the original Makefile's source list, so it isn't in the
> current one either. If the link fails on `CPVMStream`, add it to `SRCS`.
