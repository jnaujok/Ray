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

# ---------------------------------------------------------------------------
# IASLib: strings, streams, files, threads and sockets.
# Only the pieces the ray tracer needs are switched on, so no database,
# JSON or TLS libraries are required.
# ---------------------------------------------------------------------------
set(USE_THREADS          ON  CACHE BOOL "" FORCE)
set(USE_NETWORK          ON  CACHE BOOL "" FORCE)
set(USE_DATABASE         OFF CACHE BOOL "" FORCE)
set(USE_SQLITE           OFF CACHE BOOL "" FORCE)
set(USE_JSON             OFF CACHE BOOL "" FORCE)
set(USE_MEMORY_DEBUGGING OFF CACHE BOOL "" FORCE)
set(IASLIB_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_OpenSSL ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(IASLib
    GIT_REPOSITORY https://github.com/jnaujok/IASLib.git
    GIT_TAG        v3.0.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(IASLib)

# ---------------------------------------------------------------------------
# Ray tracing core, shared by every program.
#
# It is an OBJECT library on purpose: scene object and material classes
# register themselves with static constructors (DYNAMIC_IMPLEMENTS), and a
# static library would let the linker drop them.
# ---------------------------------------------------------------------------
set(RAY_CORE_SOURCES
    core/src/AmbientMaterial.cpp
    core/src/Camera.cpp
    core/src/Color.cpp
    core/src/ColorArray.cpp
    core/src/DiffuseMaterial.cpp
    core/src/ImageFile.cpp
    core/src/Light.cpp
    core/src/LightArray.cpp
    core/src/Material.cpp
    core/src/MaterialList.cpp
    core/src/Matrix.cpp
    core/src/Objects.cpp
    core/src/ObjectsArray.cpp
    core/src/Plane.cpp
    core/src/PtLight.cpp
    core/src/RayArray.cpp
    core/src/Rays.cpp
    core/src/RaySettings.cpp
    core/src/RayStream.cpp
    core/src/ReflectiveMaterial.cpp
    core/src/Scene.cpp
    core/src/SocketTaskMaster.cpp
    core/src/SocketTaskSlave.cpp
    core/src/SpecularMaterial.cpp
    core/src/Sphere.cpp
    core/src/TaskMaster.cpp
    core/src/TaskSlave.cpp
    core/src/ThreadTaskMaster.cpp
    core/src/Transform.cpp
    core/src/TransparentMaterial.cpp
    core/src/Triangle.cpp
    core/src/Vector.cpp
    core/src/WireProtocol.cpp
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
# Tests
# ---------------------------------------------------------------------------
if(RAY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
