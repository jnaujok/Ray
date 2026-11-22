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
v3.0. It is made of four programs and a shared core:

| Part | Location | Platform |
|------|----------|----------|
| Ray tracing core: scene, objects (including meshes, Bezier patches and B-spline surfaces), materials, samplers, color models, JSON scene files, wire protocol, task masters | `core/` | all |
| `raythrower`: command-line master (threads, or engines over TCP) | `raythrower/` | Windows, Linux, macOS |
| `rayengine`: networked tracing task | `rayengine/` | Windows, Linux, macOS |
| `ray-editor`: cross-platform scene editor (wxWidgets) | `editor/` (`logic/` toolkit-free, `gui/` wxWidgets) | Windows, Linux, macOS |
| `Ray.exe`: MFC scene editor | `src/`, `include/`, `res/` | Windows only (MFC, MBCS) |

The core is compiled twice: once as the `ray_core` OBJECT library for the command-line programs
and `ray-editor`, and again inside the MFC target with `RAY_GUI` defined, which turns on the
tree-view and dialog code in the scene classes. Keep `ray_core` an OBJECT library: object and
material classes register themselves from static constructors, which a static library would let
the linker drop.

`ray-editor` uses `ray_core` without `RAY_GUI`: it must never include MFC headers or use the
`RAY_GUI` code. Its logic (`editor/logic/`, the `ray_editor_logic` OBJECT library: document,
undo, tessellation, the view's camera, picking, the progressive render) includes no wxWidgets
headers, is always built, and is tested by `editor_test` without a display; only `editor/gui/`
uses wxWidgets. Its view's camera must aim rays exactly as `CCamera::RayDirection` does (the
scene's coordinates are left-handed); `editor_test` checks it. The view uses OpenGL 1.x
fixed-function calls only, so it runs on Windows' `opengl32`, macOS's legacy profile and Mesa.

## Building and testing

```sh
cmake --preset release          # or: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build --preset release
ctest --preset fast             # every test but the slow ones, 8 at a time
ctest --preset full             # every test
```

- IASLib is fetched by CMake (`FetchContent`, pinned to commit `de038c0`, the one that added
  SJSON, until IASLib tags a release with it). To use a local checkout, pass
  `-DFETCHCONTENT_SOURCE_DIR_IASLIB=/path/to/IASLib`. SJSON's LZ4 and zstd support is kept off.
- The GUI needs MSVC with MFC: `-DRAY_BUILD_GUI=ON`, then build the `Ray` target.
- The cross-platform editor: `-DRAY_BUILD_EDITOR=ON`, then build the `ray-editor` target. CMake
  uses an installed wxWidgets (`find_package`) or fetches 3.2.9 (pinned by SHA-256 in
  `cmake/RayEditor.cmake`) and builds only `base`, `core`, `gl` and `propgrid`, static. Linux needs
  `libgtk-3-dev` and `libgl1-mesa-dev`. `-DRAY_WXWIDGETS_URL=file:///…` fetches a local tarball.
  New editor source files go in `cmake/RayEditor.cmake` (GUI) or `editor/CMakeLists.txt` (logic).
- Time renders with a Release build (`-DCMAKE_BUILD_TYPE=Release`); a Debug build is several
  times slower and misleads.
- **Testing workflow.** Test optimized builds, as CI does: the Release preset for the tests and
  the `asan` preset (RelWithDebInfo with AddressSanitizer and UndefinedBehaviorSanitizer) for the
  sanitizers. A Debug or unoptimized sanitizer build runs the suite several times slower; keep it
  for the debugger. Always run ctest in parallel (`-j 8`, or the presets).
  - While working, run the tests you touched (`-R <regex>`) and the fast tier (`ctest --preset
    fast`, about a minute).
  - Before merging, run every test once (`ctest --preset full`) and the fast tier under the
    sanitizers (`ctest --preset asan-fast`) when the change touches `core/`. Don't rerun a suite
    that passed unless the code changed since.
  - CI runs every test on every system, and every test under the sanitizers, except the torture
    test (labelled `torture`, too heavy for the runners); a change merges only
    when all its jobs pass.
  - Tests that take minutes get `LABELS slow`. Keep new tests small: a render test needs only an
    image big enough to show its feature, and a networked test compares engines with threads at
    `RAY_NETWORK_SIZE` (`ray_network_test` in `tests/CMakeLists.txt`), never at full size.
- CMake compiles through `ccache` when it's installed (`-DRAY_USE_CCACHE=OFF` to stop it).
- **Every change that alters rendered output gets before and after images in `CHANGELOG.md`:** a
  side-by-side PNG in `docs/images/changelog/` (before on the left, after on the right, each
  labelled, for the scenes that changed most), linked under the change's entry with a line saying
  what to look for. Make the "before" from the reference renders in git history.
- **Every new feature goes into the torture test** (`scenes/torture_test.ray`): add it there in the
  same change, and regenerate its reference render, README preview and the full-size
  `docs/images/torture_test.png`. Render the full-size image with a Release build and the adaptive
  sampler, always the same way: `raythrower scenes/torture_test.ray --aa
  adaptive:min=16,max=100,threshold=0.004,confidence=0.95,channels=rgb --png docs/images/torture_test.png` (the `render_torture`
  test keeps the scene's own 5×5 stratified sampler at 160×120). Report its time in the changelog.
  The `render_torture` test is labelled `torture`, and CI leaves it out (`ctest -LE torture`):
  run it locally (the `full` preset does) before every merge.
- **Every release updates the release gallery** (the claude.ai artifact
  https://claude.ai/artifact/XScoVjKwNUscrc3fhGN35Z): add the release's full-size torture test to its
  comparison slider and a section with the release's summary and changelog before and after images,
  and move the release out of "Coming releases". Read the artifact back first and build on what's
  there.
- If a change is meant to alter rendered output, regenerate `tests/reference/renders.tar.gz`,
  then run `tools/render_previews.py` to update the README's expected-output images
  (`docs/images/tests/`), and say so in the changelog. Otherwise a reference mismatch is a bug.
  A new reference render also needs a place in the README's "Expected output" tables.

## Rules for the code

- **Don't change the ray tracing logic** (intersection, shading, primary ray generation in
  `CRay::CreateRay`, `CCamera::RayDirection`, `CCamera::PrimaryRay`, `CLens`, and the pixel
  samplers) without being asked to. A camera without a lens must stay an exact pinhole.
  Fixing crashes and undefined behavior is fine.
- **Saved scenes are IASLib resource archives** (`CResourceFile`): `scene.json` (folder 1) and
  every file the scene uses (folder 2), under the names the scene gives them. Plain JSON `.ray`
  files load too, and the sample scenes in `scenes/` stay JSON so diffs stay readable;
  `--save-json` writes JSON with the files extracted beside it. Read a file a scene names with
  `RayJson::ReadSceneFile` (archive first, then disk) and write its name with
  `RayJson::SceneFileName`, so it goes into the archive, never with plain `fopen`.
- **The scene itself is JSON** (version 3; documented in `scenes/README.md`). Each scene class
  reads and writes its own fields in `ToJson`/`FromJson`, through the `RayJson` helpers in
  `SceneJson.h`; readers reject unknown fields and report errors with `RayJson::Fail`. Keep the
  format backward compatible: add optional fields with defaults, and bump `JSON_VERSION` in
  `Scene.cpp` only for a change old readers would get wrong. Update `scenes/README.md` with it.
- **The 1997 binary scene format is gone,** along with `CRayStream`. Don't bring back binary
  scene files.
- **Performance work keeps images byte for byte the same** unless a change is meant to alter them:
  check with `tools/bench.py` (it prints each image's MD5) and the reference renders, and record
  what was measured in `docs/performance.md`. No fast-math or other flags that reorder
  floating-point arithmetic.
- **The wire** (`WireProtocol.h`): the scene goes as SJSON; tiles and their pixels as fixed-layout
  big-endian numbers, the pixels as IEEE-754 doubles. Numbers must cross exactly, so networked
  engines match threads byte for byte; the `network_*` tests, `engine_churn` and `format_test`'s
  wire checks hold you to it. Bump `RAY_WIRE_VERSION` for any change.
- **The work unit is a tile.** Tasks (threads or engines) pull tiles from the master's queue and
  trace them with `CTileTracer`, which runs the camera's sampler and lens for the tile, so camera
  rays are made in parallel. The master only hands out tiles and writes pixels. Splitting the image
  differently must not change it: samplers sum in fixed point, and a ray shared across a tile edge
  (a corner) is traced by both tiles and comes out the same.
- **If JSON output changes**, re-save the sample scenes with
  `raythrower scenes/<scene>.ray --save scenes/<scene>.ray` and check the renders still match.
- Use IASLib for strings, streams, files, threads, sockets, and JSON rather than adding new
  hand-written versions. In core headers write `IASLib::CString` in full: the GUI also sees MFC's
  `CString`.
- Avoid names that are Win32 macros (`SendMessage`, `GetMessage`, `CreateWindow`, ...).
- **Never call `min` or `max` unparenthesized.** Windows headers define `min` and `max` as macros,
  and the MFC build sees them, so `std::min(a, b)`, `std::max(...)` and
  `std::numeric_limits<T>::max()` break the Windows GUI build. Write `(std::min)(a, b)`,
  `(std::max)(a, b)` and `(std::numeric_limits<T>::max)()` in every file under `core/`, `src/`,
  `include/` and `editor/`. The `lint_minmax` test checks this on every platform.

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
  attached to the camera, listed in the table in `core/src/PixelSampler.cpp`. They sample one
  tile at a time (`Begin( left, top, width, height )`) in image coordinates; keep sampling inside
  the samplers, not in `CTaskMaster`, `CTileTracer` or the transports.
- **New ways of turning pixel colors into bytes** (gamma, tone mapping, and the like) are
  `CColorModel` subclasses attached to the camera, listed in the table in
  `core/src/ColorModel.cpp`. Samplers hand linear, unclipped colors to the `CPixelWriter`.
- **New textures** are `CTexture` subclasses listed in the table in `core/src/Texture.cpp`.
  Materials read their color through `CMaterial::ColorAt`, so a texture applies to any of them.
  Anything a texture reads from a file must also be written into the scene when
  `RayJson::EmbeddingData()` is set, since engines may not have the file.
- **Objects that can be bounded** override `CObjects::GetBounds` (a box a little larger than the
  shape, since some hit tests allow a hair outside it) so the scene's tree can skip them, and
  objects made of parts override `Occludes` to stop at the first part a shadow ray hits. The tree
  must find exactly what testing every object in order does (`ClosestHitByEachObject`), ties
  included; `format_test` checks it.
- **New metals** go in the table in `core/src/MetalMaterial.cpp` (n and k at about 650, 550 and
  450 nm, with their source).
- **Model files** are read in `core/src/ModelFile.cpp` (STL, OBJ), `core/src/GltfFile.cpp`
  (glTF 2.0, with Draco through `core/src/DracoDecoder.cpp`) and `core/src/BlendFile.cpp` (Blender
  `.blend`, unpacked by `core/src/Decompress.cpp`). New formats are read in Ray, not by converting
  them with a script; a mesh reads every file a model names (a `.gltf`'s buffers) through
  `RayJson::ReadSceneFile` and writes it back with `RayJson::SceneFileName`, so it goes into the
  archive and to engines.
- **New film stocks** go in the table in `core/src/FilmColorModel.cpp`, **new camera bodies** in
  `CCamera`'s table in `core/src/Camera.cpp`, and **new Apollo lens prescriptions** in
  `tools/apollo_lenses.py`, which writes them into `core/src/Lens.cpp` (`--write core/src/Lens.cpp`)
  between its markers; edit the script, not the generated tables. A color model that needs the whole
  image (grain, the spread of light) says so with `NeedsWholeImage` and does it in `Finish`, which
  `CTaskMaster::TraceScene` calls once the image is traced.
- **New material presets** go in the table in `core/src/Presets.cpp`: a texture as JSON and,
  unless it is a texture only, a finish (a JSON array of materials whose textures name the preset).
  Read and write textures with `CTexture::Read` and `CTexture::Write`, never `CTexture::Create`
  and `ToJson` by hand, so presets work wherever a texture goes. List every preset in
  `scenes/README.md`.
- **Material and light settings are maps:** a new number or color setting is a `CScalarMap` or
  `CColorMap` (`Maps.h`), read with `Read`, written with `Write` and evaluated with `At` at the
  shading point (a `CMapPoint`), so a texture can drive it. Give a scalar map its valid range with
  `SetRange`, and check a constant's range only when it isn't mapped. A constant must save and
  shade exactly as a plain number did.
- **New object, light, or material classes** register a JSON `type` name in
  `DYNAMIC_IMPLEMENTS( CClass, "name" )` or `IMPLEMENT_MATERIAL( CClass, Name, "name" )`, and the
  macros make them implement `ToJson` and `FromJson`.
- **New shapes worked out in a frame of their own** (standing on +y, turned by a `rotation`) derive
  from `CLocalShape`: answer `LocalHit`, `LocalNormal`, `LocalCoords`, `LocalBounds` and
  `LocalTriangles` (for `ray-editor`'s preview) in that frame, and list their own JSON fields; the
  base class turns rays and points, and reads and writes `center`, `rotation`, `color`, `materials`
  and `bump`. Add the type to `editor/logic` (`Tessellator.cpp`, `EditorDocument.cpp`) too.
- **Adding a source file:** add it to `RAY_CORE_SOURCES` in `CMakeLists.txt` (core) or to
  `cmake/RayGui.cmake` (GUI).
- **Don't modify** `legacy/` (historical project files).

## Repository policy

- Licensed under Apache 2.0. Keep `LICENSE` and `NOTICE` intact. IASLib is LGPL-3.0.
- Outside contributions are closed (see `CONTRIBUTING.md`). Don't open or accept pull requests
  from people outside the project.

## Known issues

- The MFC application can only be built on Windows; on Linux and macOS the core, the command-line
  programs and `ray-editor` build.
