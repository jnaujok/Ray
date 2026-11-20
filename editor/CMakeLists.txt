# The cross-platform scene editor.
#
# Its logic (the document, tessellation, the view's camera, picking and the
# progressive render) needs no window toolkit, so it is always built, and
# tests/editor_test checks it without a display. The wxWidgets program
# itself, ray-editor, is built only with -DRAY_BUILD_EDITOR=ON.

add_library(ray_editor_logic OBJECT
    logic/EditorDocument.cpp
    logic/RenderJob.cpp
    logic/Tessellator.cpp
    logic/ViewCamera.cpp
)
target_include_directories(ray_editor_logic PUBLIC logic)
target_link_libraries(ray_editor_logic PUBLIC ray_core)
find_package(Threads REQUIRED)
target_link_libraries(ray_editor_logic PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(ray_editor_logic PRIVATE /W3)
else()
    target_compile_options(ray_editor_logic PRIVATE -Wall)
endif()

if(RAY_BUILD_EDITOR)
    include(${PROJECT_SOURCE_DIR}/cmake/RayEditor.cmake)
endif()
