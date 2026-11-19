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

Ray is a distributed ray tracer from 1997, made of three programs:

| Program | Location | Platform | Build |
|---------|----------|----------|-------|
| `Ray.exe`: MFC scene editor / master | `src/`, `include/`, `res/` | Windows (MFC, MBCS) | `Ray.dsw` (VC++ 6) or CMake with MSVC |
| `rayengine`: PVM worker | `rayengine/` | Unix + PVM 3 | `rayengine/Makefile` or CMake `-DRAY_BUILD_PVM_TOOLS=ON` |
| `raythrower`: command-line PVM master | `raythrower/` | Unix + PVM 3 | `raythrower/Makefile` or CMake |

`rayengine/` has its own fork of the core ray tracing classes (`Scene`, `Sphere`, materials, and
so on), and they have drifted from the copies in the root `src/` and `include/`. Changing one copy
doesn't change the other. Check both and decide deliberately.

## Conventions

- **File names:** source and header files are PascalCase after the class they define, without
  the MFC `C` prefix (`CSpherePropertiesDlg` → `SpherePropertiesDlg.h`). Exceptions follow
  Windows convention: `StdAfx.cpp`/`StdAfx.h`, `resource.h`, `MainFrm`, `ChildFrm`, and `main.cpp`.
  Data and scene files are snake_case. PVM headers keep their upstream lowercase names.
- **Extensions:** `.cpp` and `.h`.
- **Includes:** `#include` names must match the file name's case exactly, because Linux is
  case-sensitive. Use include paths rather than `..\` or `../` relative includes.
- **Line endings:** LF everywhere except `*.dsp`, `*.dsw`, `*.clw`, and `*.bat`, which must stay
  CRLF because Visual C++ 6 and `cmd.exe` need it. `.gitattributes` enforces this.
- **Adding a source file:** add it to `CMakeLists.txt`, and to `Ray.dsp` or the right `Makefile`.
- **Don't modify** `legacy/` (historical project files) or `third_party/` (bundled PVM 3).

## Repository policy

- Licensed under Apache 2.0. Keep `LICENSE` and `NOTICE` intact.
- Outside contributions are closed (see `CONTRIBUTING.md`). Don't open or accept pull requests
  from people outside the project.

## Known issues

- The Unix sources don't compile cleanly with a modern GCC or Clang (`TRUE`/`FALSE` and other
  1997-era assumptions).
- The MFC application can only be built on Windows. On Linux, at most you can check includes
  and syntax.
