# CLAUDE.md

Guidance for Claude Code when working in this repository.

## Required on every check-in

**Every commit must keep `CHANGELOG.md` and `README.md` up to date.**

- Add an entry under `## [Unreleased]` in `CHANGELOG.md` for each change, using the Keep a
  Changelog sections (`Added`, `Changed`, `Fixed`, `Removed`, `Deprecated`, `Security`).
- Update `README.md` whenever a change affects anything it describes: features, layout, build
  steps, usage, status, or badges. If a sub-project changes, update `rayengine/README.md` or
  `raythrower/README.md` too.
- If a change really doesn't affect the README, say so in the commit message rather than skipping
  the check silently.
- Don't commit until both files have been reviewed and updated.

## Project overview

Ray is a distributed ray tracer from 1997, rebuilt on [IASLib](https://github.com/jnaujok/IASLib)
v3.0. It is made of three programs and a shared core:

| Part | Location | Platform |
|------|----------|----------|
| Ray tracing core: scene, objects (including meshes), materials, samplers, color models, JSON scene files, wire protocol, task masters | `core/` | all |
| `raythrower`: command-line master (threads, or engines over TCP) | `raythrower/` | Windows, Linux, macOS |
| `rayengine`: networked tracing task | `rayengine/` | Windows, Linux, macOS |
| `Ray.exe`: MFC scene editor | `src/`, `include/`, `res/` | Windows only (MFC, MBCS) |

The core is compiled twice: once as the `ray_core` OBJECT library for the command-line programs,
and again inside the GUI target with `RAY_GUI` defined, which turns on the tree-view and dialog
code in the scene classes. Keep `ray_core` an OBJECT library: object and material classes register
themselves from static constructors, which a static library would let the linker drop.

## Building and testing

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

- IASLib is fetched by CMake (`FetchContent`, pinned to commit `de038c0`, the one that added
  SJSON, until IASLib tags a release with it). To use a local checkout, pass
  `-DFETCHCONTENT_SOURCE_DIR_IASLIB=/path/to/IASLib`. SJSON's LZ4 and zstd support is kept off.
- The GUI needs MSVC with MFC: `-DRAY_BUILD_GUI=ON`, then build the `Ray` target.
- Run the tests under AddressSanitizer before changing anything in `core/`
  (`-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`); CI does.
- If a change is meant to alter rendered output, regenerate `tests/reference/renders.tar.gz`,
  then run `tools/render_previews.py` to update the README's expected-output images
  (`docs/images/tests/`), and say so in the changelog. Otherwise a reference mismatch is a bug.
  A new reference render also needs a place in the README's "Expected output" tables.

## Rules for the code

- **Don't change the ray tracing logic** (intersection, shading, primary ray generation in
  `CRay::CreateRay`, `CCamera::RayDirection`, `CCamera::PrimaryRay`, `CLens`, and the pixel
  samplers) without being asked to. A camera without a lens must stay an exact pinhole.
  Fixing crashes and undefined behavior is fine.
- **Scene files are JSON** (`.ray`, version 3; documented in `scenes/README.md`). Each scene class
  reads and writes its own fields in `ToJson`/`FromJson`, through the `RayJson` helpers in
  `SceneJson.h`; readers reject unknown fields and report errors with `RayJson::Fail`. Keep the
  format backward compatible: add optional fields with defaults, and bump `JSON_VERSION` in
  `Scene.cpp` only for a change old readers would get wrong. Update `scenes/README.md` with it.
- **The 1997 binary scene format is gone,** along with `CRayStream`. Don't bring back binary
  scene files.
- **The wire** (`WireProtocol.h`): the scene goes as SJSON, ray and color batches as fixed-layout
  big-endian IEEE-754 doubles, for speed (SJSON costs tens of microseconds a ray to encode). Numbers
  must cross exactly, so networked engines match threads byte for byte; the `network_*` tests and
  `format_test`'s wire checks hold you to it. Bump `RAY_WIRE_VERSION` for any change.
- **If JSON output changes**, re-save the sample scenes with
  `raythrower scenes/<scene>.ray --save scenes/<scene>.ray` and check the renders still match.
- Use IASLib for strings, streams, files, threads, sockets, and JSON rather than adding new
  hand-written versions. In core headers write `IASLib::CString` in full: the GUI also sees MFC's
  `CString`.
- Avoid names that are Win32 macros (`SendMessage`, `GetMessage`, `CreateWindow`, ...).

## Conventions

- **File names:** source and header files are PascalCase after the class they define, without
  the MFC `C` prefix (`CSpherePropertiesDlg` → `SpherePropertiesDlg.h`). Exceptions follow
  Windows convention: `StdAfx.cpp`/`StdAfx.h`, `resource.h`, `MainFrm`, `ChildFrm`, and `main.cpp`.
  Data and scene files are snake_case.
- **Extensions:** `.cpp` and `.h`.
- **Includes:** `#include` names must match the file name's case exactly, because Linux is
  case-sensitive. Use include paths rather than `..\` or `../` relative includes.
- **Line endings:** LF everywhere except `*.dsp`, `*.dsw`, `*.clw`, and `*.bat`, which must stay
  CRLF because Visual C++ 6 and `cmd.exe` need it. `.gitattributes` enforces this.
- **New ways of sampling pixels** (anti-aliasing and the like) are `CPixelSampler` subclasses
  attached to the camera, listed in the table in `core/src/PixelSampler.cpp`. Keep sampling out of
  `CTaskMaster` and the engines.
- **New ways of turning pixel colors into bytes** (gamma, tone mapping, and the like) are
  `CColorModel` subclasses attached to the camera, listed in the table in
  `core/src/ColorModel.cpp`. Samplers hand linear, unclipped colors to the `CPixelWriter`.
- **New textures** are `CTexture` subclasses listed in the table in `core/src/Texture.cpp`.
  Materials read their color through `CMaterial::ColorAt`, so a texture applies to any of them.
  Anything a texture reads from a file must also be written into the scene when
  `RayJson::EmbeddingData()` is set, since engines may not have the file.
- **New object, light, or material classes** register a JSON `type` name in
  `DYNAMIC_IMPLEMENTS( CClass, "name" )` or `IMPLEMENT_MATERIAL( CClass, Name, "name" )`, and the
  macros make them implement `ToJson` and `FromJson`.
- **Adding a source file:** add it to `RAY_CORE_SOURCES` in `CMakeLists.txt` (core) or to
  `cmake/RayGui.cmake` (GUI).
- **Don't modify** `legacy/` (historical project files).

## Repository policy

- Licensed under Apache 2.0. Keep `LICENSE` and `NOTICE` intact. IASLib is LGPL-3.0.
- Outside contributions are closed (see `CONTRIBUTING.md`). Don't open or accept pull requests
  from people outside the project.

## Known issues

- Meshes are slow: every ray inside a mesh's bounding box tests every face (there is no
  acceleration structure within a mesh yet).
- The MFC application can only be built on Windows; on Linux and macOS only the core and the
  command-line programs build.
