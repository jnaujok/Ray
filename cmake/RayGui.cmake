# The Windows MFC scene editor (Ray.exe). Included from the top-level
# CMakeLists.txt when RAY_BUILD_GUI is on.
#
# The core is compiled again here with RAY_GUI defined: in this build the
# scene classes also carry their tree-view and property-dialog code.

set(CMAKE_MFC_FLAG 2)   # MFC in a shared DLL (_AFXDLL)

set(RAY_GUI_SOURCES
    src/AddMaterialsDlg.cpp
    src/AddPlaneDlg.cpp
    src/AddPtLightDlg.cpp
    src/AddSphereDlg.cpp
    src/CameraPropertiesDlg.cpp
    src/ChildFrm.cpp
    src/DiffuseMaterialPropDlg.cpp
    src/InfluenceDlg.cpp
    src/JsonEditDlg.cpp
    src/MainFrm.cpp
    src/MaterialsDlg.cpp
    src/PlanePropertiesDlg.cpp
    src/PointLightPropertiesDlg.cpp
    src/Ray.cpp
    src/RayDoc.cpp
    src/RayView.cpp
    src/ReflectiveMatDlg.cpp
    src/SceneTreeView.cpp
    src/SetColorDlg.cpp
    src/SettingsDialog.cpp
    src/SpecMatDlg.cpp
    src/SpherePropertiesDlg.cpp
    src/StdAfx.cpp
    src/TransparentMatDlg.cpp
    res/Ray.rc
)
# src/SceneTreeDlg.cpp was never part of the build (it is not in Ray.dsp either).

add_executable(Ray WIN32 ${RAY_GUI_SOURCES} ${RAY_CORE_SOURCES})
target_include_directories(Ray PRIVATE include core/include res)
target_compile_definitions(Ray PRIVATE RAY_GUI _AFXDLL _MBCS _CRT_SECURE_NO_WARNINGS
                                       $<$<CONFIG:Debug>:_DEBUG>)
target_compile_options(Ray PRIVATE /W3)
target_link_libraries(Ray PRIVATE IASLib::IASLib)
