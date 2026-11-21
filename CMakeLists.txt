cmake_minimum_required(VERSION 3.18)
project(Ray VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(RAY_BUILD_GUI   "Build the Windows MFC scene editor (MSVC only)" OFF)
option(RAY_BUILD_TESTS "Build the regression tests" ON)
option(RAY_BUILD_EDITOR "Build the cross-platform wxWidgets scene editor (ray-editor)" OFF)

# ---------------------------------------------------------------------------
# IASLib: strings, files, threads, sockets, JSON (scene files) and SJSON (the
# compact binary JSON the master and engines talk in). Only the pieces the ray
# tracer needs are switched on, so no database or TLS libraries are required.
# SJSON can use liblz4 and libzstd when it finds them; they are kept out so
# every build, and every build's network packets, are the same.
# ---------------------------------------------------------------------------
set(USE_THREADS          ON  CACHE BOOL "" FORCE)
set(USE_NETWORK          ON  CACHE BOOL "" FORCE)
set(USE_DATABASE         OFF CACHE BOOL "" FORCE)
set(USE_SQLITE           OFF CACHE BOOL "" FORCE)
set(USE_JSON             ON  CACHE BOOL "" FORCE)
set(USE_SJSON            ON  CACHE BOOL "" FORCE)
set(LZ4_INCLUDE_DIR      "" CACHE PATH "" FORCE)
set(LZ4_LIBRARY          "" CACHE FILEPATH "" FORCE)
set(ZSTD_INCLUDE_DIR     "" CACHE PATH "" FORCE)
set(ZSTD_LIBRARY         "" CACHE FILEPATH "" FORCE)
set(USE_MEMORY_DEBUGGING OFF CACHE BOOL "" FORCE)
set(IASLIB_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_OpenSSL ON CACHE BOOL "" FORCE)

include(FetchContent)
# SJSON came after the v3.0.0 tag, so this pins the commit that added it
# (de038c0, 2026-11-18) until there is a release tag to use.
FetchContent_Declare(IASLib
    GIT_REPOSITORY https://github.com/jnaujok/IASLib.git
    GIT_TAG        de038c077de025696015f3a3b70230f4380bb8bb)
FetchContent_MakeAvailable(IASLib)

# ---------------------------------------------------------------------------
# Ray tracing core, shared by every program.
#
# It is an OBJECT library on purpose: scene object and material classes
# register themselves with static constructors (DYNAMIC_IMPLEMENTS), and a
# static library would let the linker drop them.
# ---------------------------------------------------------------------------
set(RAY_CORE_SOURCES
    core/src/AdaptiveSampler.cpp
    core/src/AmbientMaterial.cpp
    core/src/Box.cpp
    core/src/BumpMap.cpp
    core/src/BoxSampler.cpp
    core/src/Camera.cpp
    core/src/CausticsTexture.cpp
    core/src/CellsTexture.cpp
    core/src/CheckerTexture.cpp
    core/src/CenterSampler.cpp
    core/src/ClipColorModel.cpp
    core/src/Color.cpp
    core/src/Cone.cpp
    core/src/Csg.cpp
    core/src/ColorModel.cpp
    core/src/CornerSampler.cpp
    core/src/Cylinder.cpp
    core/src/DiffuseMaterial.cpp
    core/src/FaceColorTexture.cpp
    core/src/GammaColorModel.cpp
    core/src/HdrColorModel.cpp
    core/src/ImageExport.cpp
    core/src/ImageFile.cpp
    core/src/ImageWriter.cpp
    core/src/ImageTexture.cpp
    core/src/LayersTexture.cpp
    core/src/Lens.cpp
    core/src/Light.cpp
    core/src/LightArray.cpp
    core/src/MarbleTexture.cpp
    core/src/Material.cpp
    core/src/MaterialList.cpp
    core/src/Matrix.cpp
    core/src/Mesh.cpp
    core/src/ModelFile.cpp
    core/src/NoiseTexture.cpp
    core/src/Objects.cpp
    core/src/ObjectsArray.cpp
    core/src/PixelAccumulator.cpp
    core/src/PixelSampler.cpp
    core/src/PngImage.cpp
    core/src/Presets.cpp
    core/src/Plane.cpp
    core/src/PtLight.cpp
    core/src/Rays.cpp
    core/src/RandomSampler.cpp
    core/src/RaySettings.cpp
    core/src/RectangleLight.cpp
    core/src/ReflectiveMaterial.cpp
    core/src/ReinhardColorModel.cpp
    core/src/Scene.cpp
    core/src/SceneJson.cpp
    core/src/SocketTaskMaster.cpp
    core/src/SolidTexture.cpp
    core/src/SocketTaskSlave.cpp
    core/src/SpecularMaterial.cpp
    core/src/Sphere.cpp
    core/src/SphereLight.cpp
    core/src/Spectrum.cpp
    core/src/SpotLight.cpp
    core/src/DirectionalLight.cpp
    core/src/Environment.cpp
    core/src/Haze.cpp
    core/src/SrgbColorModel.cpp
    core/src/StratifiedSampler.cpp
    core/src/Texture.cpp
    core/src/TaskMaster.cpp
    core/src/TaskSlave.cpp
    core/src/ThreadTaskMaster.cpp
    core/src/TileTracer.cpp
    core/src/Transform.cpp
    core/src/TransparentMaterial.cpp
    core/src/Triangle.cpp
    core/src/Vector.cpp
    core/src/WireProtocol.cpp
    core/src/WoodTexture.cpp
)

add_library(ray_core OBJECT ${RAY_CORE_SOURCES})
target_include_directories(ray_core PUBLIC core/include)
target_link_libraries(ray_core PUBLIC IASLib::IASLib)
if(MSVC)
    # CMake adds /D_WINDOWS to every MSVC build; the core keys its GUI code on RAY_GUI instead.
    target_compile_definitions(ray_core PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_compile_options(ray_core PRIVATE /W3)
else()
    target_compile_options(ray_core PRIVATE -Wall)
endif()

# ---------------------------------------------------------------------------
# Command-line programs (Windows, Linux, macOS)
# ---------------------------------------------------------------------------
add_executable(raythrower raythrower/main.cpp)
target_link_libraries(raythrower PRIVATE ray_core)

add_executable(rayengine rayengine/main.cpp)
target_link_libraries(rayengine PRIVATE ray_core)

install(TARGETS raythrower rayengine RUNTIME DESTINATION bin)

# ---------------------------------------------------------------------------
# Windows MFC scene editor
# ---------------------------------------------------------------------------
if(RAY_BUILD_GUI)
    if(NOT MSVC)
        message(FATAL_ERROR "RAY_BUILD_GUI needs MSVC with MFC")
    endif()
    include(cmake/RayGui.cmake)
endif()

# ---------------------------------------------------------------------------
# Cross-platform scene editor (wxWidgets): its logic always, the program with
# RAY_BUILD_EDITOR
# ---------------------------------------------------------------------------
add_subdirectory(editor)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(RAY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
