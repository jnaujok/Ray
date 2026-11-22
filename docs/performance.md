# Performance

Where Ray spends its time, as measured for 2026-alpha-0.21, and what was done about it. Every
change below leaves every image byte for byte the same (the reference renders and the network tests
check it), except the one marked as a render change.

## How it was measured

- **Profiles:** `perf` isn't allowed on the machine used (`perf_event_paranoid` is 4), so the
  profiles come from `gprof` on the original code and from a small sampling profiler (a preloaded
  library that records a backtrace every 2 ms of CPU) on Release builds with symbols.
- **Timings:** `tools/bench.py` renders six fixed cases with 16 threads and reports wall time, CPU
  time (user + system), peak memory and an MD5 of the image. The machine was shared with other work
  (load 6 to 19 on 16 hardware threads), so CPU times carry a few percent of noise and wall times
  more; each figure is the faster of two runs, and before and after were run back to back.

## Before: where the time went

- **The torture test** (240×180, the full-size adaptive settings): 40% of all CPU was one function,
  `CLocalShape::PolynomialRoots`, nearly all of it for the two lathes (the goblet and the queen).
  Most of that came through shadow rays: the floor's area-light shadow rays pass near the lathes,
  and each one solved a degree-6 polynomial for every profile piece it came near, bisecting each
  root to neighboring doubles (about 52 steps a root, 20 million roots at 80×60). Then the Bézier
  teapot (13%), the scene tree's box tests (4%), the mesh tree's node tests (3%) and noise and cell
  textures (5%).
- **The tail of a frame:** at 240×180 there are 48 tiles of 32 pixels; the last few, over the
  lathes, kept one or two threads busy while the rest sat idle, so 16 threads ran only 7.5 times
  faster than one.
- **The chessboard:** 31% of its CPU was reading the scene, not tracing it. Each of the 16 worker
  threads gets its own copy of the scene as JSON text, with the chess set's glTF buffers in it as
  base64, and each decoded them again (IASLib's decoder pushes one byte at a time): 18% of all CPU.
  Hashing the decoded model for the shared-geometry cache, a byte at a time, was another 4%.
- **Networked engines:** the master spent 48% of its time in IASLib's SJSON bit writer, encoding
  the scene once for every engine (each connection was its own SJSON stream, so each needed its own
  encoding). With Benchy that is about 2.4 s an engine.

## What changed

| Change | Effect | Images |
|---|---|---|
| End-of-frame tile splitting: once fewer tiles are waiting than twice the tasks, the next tiles are split into quarters, down to 8 pixels | torture test wall time −30% (153 s to 108 s at 240×180); a pixel is traced the same in any tile | identical |
| Decoded file data shared between the scene copies (`RayJson::BinaryValue`: the last few decoded blobs kept by a hash of their text) | chessboard CPU −21% (122 s to 96 s), wall −20% | identical |
| A word-at-a-time hash for the model and file caches (`RayJson::HashBytes`) | part of the chessboard's gain | identical |
| The scene's SJSON packet encoded once a trace and sent to every engine: each scene now starts a fresh stream (sequence 0), so the packet depends only on the scene. Wire version 5 | networked render wall −33% (8.5 s to 5.6 s), master CPU −36% | identical |
| Lathe shadow rays stop at the first hit (`CLathe::Occludes`, as Bézier patches already did), except for a ray leaving the lathe itself, where the nearest hit decides | about 2% of the torture test | identical |
| A lathe's nearest-point search (for normals and surface coordinates) remembers its last answer and skips pieces whose control-point box is farther than the best so far, with room for rounding | about 1% | identical |
| The root finder first checks, in the Bernstein basis, whether a polynomial is certainly clear of zero over the whole range (then it can have no root, and rounding can't fake one); and once a bracket is small, it certifies (derivative of one sign, values well clear of zero at points either side of the root) which way most of the remaining bisection steps go, so only the last dozen evaluate the polynomial | evaluations down from about 52 to 18 a root, but the function got only 8% faster: the bisection is bound by its chain of dependent steps, not by evaluating | identical |
| Link-time optimization (`CMAKE_INTERPROCEDURAL_OPTIMIZATION`) | 2–3% CPU, within noise; not kept, as it lengthens every build | identical |
| Objects-method bounce light: an object whose bounds hold the shading point is gathered over the whole half above it, and now gets four times `samples` rays there | fixes the speckles on the AS11 suit with the objects method; costs the torture test about 22% more CPU, since its big boxes' bounding spheres hold much of the scene | **render change** |

## After

Six cases, 16 threads, the faster of two runs, 2026-alpha-0.20.2 against 0.21 back to back:

| Case | Wall before | Wall after | CPU before | CPU after | Peak memory before / after |
|---|---|---|---|---|---|
| Torture test, 240×180, adaptive | 153.4 s | 108.0 s | 1,134 s | 1,387 s ¹ | 1,031 / 1,144 MB |
| Benchy, 320×240 | 2.16 s | 2.11 s | 19.0 s | 18.8 s | 811 / 887 MB |
| Chessboard, 480×360 | 10.05 s | 8.04 s | 121.7 s | 95.7 s | 1,336 / 1,358 MB |
| Fractals, 320×240 | 0.49 s | 0.51 s | 3.05 s | 3.08 s | 7 / 7 MB |
| AS11-40-5903, 256×256 | 4.72 s | 4.38 s | 48.5 s | 47.9 s | 331 / 381 MB |
| Benchy over 3 engines, 160×120 | 8.47 s | 5.64 s | 12.5 s | 8.0 s | 285 / 224 MB |

¹ All of the torture test's extra CPU is the bounce-light change; before it, the same build took
1,113–1,125 s (and gave the old image byte for byte).

The full-size torture image (1024×768, adaptive, the command in `CLAUDE.md`) took 24,137 CPU
seconds and 34 minutes 35 seconds of wall time on the shared machine, against 19,329 CPU seconds for
2026-alpha-0.20.1: the bounce-light change's cost, plus the load on the machine.

## What's left

- The lathe root finder is still the largest single cost in the torture test. The bisection could
  stop earlier only by changing the roots it returns (and so the images), which this release didn't
  do.
- Shading points inside a large object's bounding sphere count that object as being all around
  them; a bounding box would be a tighter test for long objects like the mirror wall, and would cut
  most of the bounce-light change's cost. It changes images, so it's left for a release that may.
- The master still encodes the scene with IASLib's SJSON bit writer, about 2.4 s for Benchy; large
  embedded files could travel outside SJSON.
