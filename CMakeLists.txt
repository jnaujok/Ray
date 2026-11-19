cmake_minimum_required(VERSION 3.16)
project(Ray LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Ray - Windows MFC scene editor / front end (mirrors Ray.dsp)
# ---------------------------------------------------------------------------
if(MSVC)
    set(CMAKE_MFC_FLAG 2)   # MFC as a shared DLL (_AFXDLL)

    set(RAY_APP_SOURCES
        src/AddMaterialsDlg.cpp
        src/AddPlaneDlg.cpp
        src/AddPtLightDlg.cpp
        src/AddSphereDlg.cpp
        src/AmbientMaterial.cpp
        src/Camera.cpp
        src/CameraPropertiesDlg.cpp
        src/ChildFrm.cpp
        src/Color.cpp
        src/ColorArray.cpp
        src/DiffuseMaterial.cpp
        src/DiffuseMaterialPropDlg.cpp
        src/FileStream.cpp
        src/ImageFile.cpp
        src/InfluenceDlg.cpp
        src/Light.cpp
        src/LightArray.cpp
        src/MainFrm.cpp
        src/Material.cpp
        src/MaterialList.cpp
        src/MaterialsDlg.cpp
        src/Matrix.cpp
        src/Objects.cpp
        src/ObjectsArray.cpp
        src/PipeStream.cpp
        src/Plane.cpp
        src/PlanePropertiesDlg.cpp
        src/PointLightPropertiesDlg.cpp
        src/PtLight.cpp
        src/Ray.cpp
        src/RayArray.cpp
        src/RayDoc.cpp
        src/Rays.cpp
        src/RaySettings.cpp
        src/RayView.cpp
        src/ReflectiveMatDlg.cpp
        src/ReflectiveMaterial.cpp
        src/Scene.cpp
        src/SceneTreeView.cpp
        src/Semaphore.cpp
        src/SetColorDlg.cpp
        src/SettingsDialog.cpp
        src/SpecMatDlg.cpp
        src/SpecularMaterial.cpp
        src/Sphere.cpp
        src/SpherePropertiesDlg.cpp
        src/StdAfx.cpp
        src/TaskMaster.cpp
        src/TaskSlave.cpp
        src/ThreadTaskMaster.cpp
        src/ThreadTaskSlave.cpp
        src/Transform.cpp
        src/TransparentMatDlg.cpp
        src/TransparentMaterial.cpp
        src/Triangle.cpp
        src/Vector.cpp
        res/Ray.rc
    )
    # Not part of the original Ray.dsp build (PVM-distributed and unused code):
    #   src/PVMStream.cpp src/PVMTaskMaster.cpp src/PVMTaskSlave.cpp
    #   src/SceneTreeDlg.cpp src/TraceThread.cpp

    add_executable(Ray WIN32 ${RAY_APP_SOURCES})
    target_include_directories(Ray PRIVATE include third_party/pvm3/include res)
    target_compile_definitions(Ray PRIVATE WIN32 _WINDOWS _MBCS _AFXDLL $<$<CONFIG:Debug>:_DEBUG>)
endif()

# ---------------------------------------------------------------------------
# RayEngine / RayThrower - Unix PVM programs (also buildable via their Makefiles)
# ---------------------------------------------------------------------------
option(RAY_BUILD_PVM_TOOLS "Build the Unix PVM ray engine and ray thrower" OFF)

if(RAY_BUILD_PVM_TOOLS)
    find_library(PVM3_LIBRARY NAMES pvm3 REQUIRED)
    set(PVM3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pvm3/include"
        CACHE PATH "Directory containing pvm3.h")

    set(RAY_ENGINE_SOURCES
        AmbientMaterial Camera Color ColorArray CString DiffuseMaterial FileStream ImageFile
        Light LightArray Material MaterialList Matrix Objects ObjectsArray Plane PtLight
        RayArray Rays RaySettings ReflectiveMaterial Scene SpecularMaterial Sphere Transform
        TransparentMaterial Triangle Vector)
    list(TRANSFORM RAY_ENGINE_SOURCES PREPEND rayengine/src/)
    list(TRANSFORM RAY_ENGINE_SOURCES APPEND .cpp)

    add_executable(rayengine ${RAY_ENGINE_SOURCES} rayengine/src/main.cpp)
    target_include_directories(rayengine PRIVATE rayengine/include ${PVM3_INCLUDE_DIR})
    target_compile_definitions(rayengine PRIVATE UNIX)
    target_link_libraries(rayengine PRIVATE ${PVM3_LIBRARY} m)

    list(FILTER RAY_ENGINE_SOURCES EXCLUDE REGEX "/CString\\.cpp$")
    add_executable(raythrower ${RAY_ENGINE_SOURCES} raythrower/src/CString.cpp raythrower/src/main_unix.cpp)
    target_include_directories(raythrower PRIVATE raythrower/include include ${PVM3_INCLUDE_DIR})
    target_compile_definitions(raythrower PRIVATE UNIX)
    target_link_libraries(raythrower PRIVATE ${PVM3_LIBRARY} m)
endif()
