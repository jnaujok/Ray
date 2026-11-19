# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-11-19

The first public import of the 1997 code base, reorganized into a modern project layout.
The ray tracing code itself is unchanged.

### Added

- `CMakeLists.txt` that builds the MFC scene editor with MSVC, plus the Unix PVM engine and thrower
  behind the `RAY_BUILD_PVM_TOOLS` option.
- Project documentation: `README.md`, `rayengine/README.md`, `raythrower/README.md`,
  `CHANGELOG.md`, `CONTRIBUTING.md`, `CLAUDE.md`.
- Apache License 2.0 (`LICENSE`) and `NOTICE`.
- `.gitignore`, `.gitattributes`, and a pull request template saying outside contributions
  aren't accepted.

### Changed

- Renamed every file from DOS upper case to modern names. Sources and headers are PascalCase after
  their class name (`ADDPLANEDLG.CPP` → `AddPlaneDlg.cpp`, `TRANSFRM.H` → `Transform.h`), and data
  files are snake_case (`INSIDE SPHERE.RAY` → `inside_sphere.ray`).
- Moved the code into `src/` and `include/` directories:
  - the MFC application went to the root `src/` and `include/`,
  - `RAYENGINE/` → `rayengine/{src,include}`,
  - `RAYTHROWER/` → `raythrower/{src,include}`.
- Renamed `.CC` sources to `.cpp`. RayThrower's Unix entry point is now `main_unix.cpp`.
- Moved resources to `res/`, WinHelp sources to `help/`, scenes to `scenes/`, PVM headers and
  libraries to `third_party/pvm3/`, and Visual C++ 4/5 project files to `legacy/`.
- Rewrote `#include` directives to match the new file names exactly, so they work on
  case-sensitive file systems. Replaced the path-relative includes
  (`"rayengine\cstring.h"`, `"..\globals.h"`, `"../pvm3/include/pvm3.h"`) with include paths.
- Pointed `Ray.dsp`, `Ray.clw`, `Ray.rc`, `makehelp.bat`, and the Unix Makefiles at the new paths.
- Converted sources, headers, resource scripts, and docs from CRLF to LF line endings. The Visual
  C++ 6 project files and batch files stay CRLF.

### Known issues

- The Unix sources don't compile cleanly with a modern GCC or Clang, mainly because `TRUE` and
  `FALSE` are expected to come from the platform headers.
- `rayengine/Makefile` doesn't list `src/PVMStream.cpp`. The original Makefile left it out too.
- The project files under `legacy/` still contain the pre-reorganization paths.

## [Historical] - 1996-1999

Original development of the distributed ray tracer: the MFC scene editor, the PVM ray engine, and
the ray thrower, built with Visual C++ 4–6 on Windows and with `make` on Unix.

[Unreleased]: https://github.com/jnaujok/Ray/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/jnaujok/Ray/releases/tag/v0.1.0
