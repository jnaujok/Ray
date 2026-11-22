# The cross-platform scene editor, ray-editor (wxWidgets), included from
# editor/CMakeLists.txt when RAY_BUILD_EDITOR is on.
#
# wxWidgets comes from the system if find_package finds it (core, base, gl
# and propgrid), and is otherwise fetched and built as static libraries with
# only what the editor needs. RAY_WXWIDGETS_URL can point the fetch at a local
# copy of the release tarball.

set(RAY_WXWIDGETS_VERSION 3.2.9)
set(RAY_WXWIDGETS_URL
    "https://github.com/wxWidgets/wxWidgets/releases/download/v${RAY_WXWIDGETS_VERSION}/wxWidgets-${RAY_WXWIDGETS_VERSION}.tar.bz2"
    CACHE STRING "Where to fetch the wxWidgets release tarball from")
set(RAY_WXWIDGETS_SHA256 fb90f9538bffd6a02edbf80037a0c14c2baf9f509feac8f76ab2a5e4321f112b)
option(RAY_USE_SYSTEM_WXWIDGETS "Use an installed wxWidgets if find_package finds one" ON)

find_package(OpenGL REQUIRED)

set(RAY_WX_LIBRARIES)
if(RAY_USE_SYSTEM_WXWIDGETS)
    find_package(wxWidgets 3.1 QUIET COMPONENTS gl propgrid core base)
endif()

if(wxWidgets_FOUND)
    message(STATUS "ray-editor: using the installed wxWidgets ${wxWidgets_VERSION_STRING}")
    include(${wxWidgets_USE_FILE})
    set(RAY_WX_LIBRARIES ${wxWidgets_LIBRARIES})
else()
    message(STATUS "ray-editor: fetching wxWidgets ${RAY_WXWIDGETS_VERSION}")

    # Only what the editor uses: static base, core, gl and propgrid. The help
    # string is one word because wxWidgets passes it on unquoted when it
    # forces an option itself (wx_option_force_value).
    foreach(wxOption
            wxBUILD_SAMPLES wxBUILD_TESTS wxBUILD_DEMOS wxBUILD_BENCHMARKS wxBUILD_INSTALL
            wxUSE_WEBVIEW wxUSE_MEDIACTRL wxUSE_STC wxUSE_RIBBON wxUSE_RICHTEXT wxUSE_XRC
            wxUSE_AUI wxUSE_HTML wxUSE_WXHTML_HELP wxUSE_WEBREQUEST wxUSE_LIBTIFF wxUSE_SECRETSTORE wxUSE_LIBSDL
            wxUSE_LIBMSPACK wxUSE_XML wxUSE_GLCANVAS_EGL wxUSE_SOUND wxUSE_PRIVATE_FONTS
            wxUSE_LIBNOTIFY wxUSE_LIBGNOMEVFS wxUSE_SPELLCHECK)
        set(${wxOption} OFF CACHE BOOL "Set-by-the-Ray-editor-build" FORCE)
    endforeach()
    set(wxBUILD_SHARED OFF CACHE BOOL "Set-by-the-Ray-editor-build" FORCE)
    set(wxBUILD_PRECOMP OFF CACHE STRING "Set-by-the-Ray-editor-build" FORCE)
    set(wxUSE_OPENGL ON CACHE BOOL "Set-by-the-Ray-editor-build" FORCE)
    set(wxUSE_PROPGRID ON CACHE BOOL "Set-by-the-Ray-editor-build" FORCE)
    if(MSVC)
        # Match the rest of the build's runtime library.
        set(wxBUILD_USE_STATIC_RUNTIME OFF CACHE BOOL "Set-by-the-Ray-editor-build" FORCE)
    endif()

    # wxWidgets 3.2's CMakeLists asks for a CMake older than 3.5, which CMake 4
    # refuses unless told what policies to assume.
    if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    endif()

    FetchContent_Declare(wxWidgets
        URL      ${RAY_WXWIDGETS_URL}
        URL_HASH SHA256=${RAY_WXWIDGETS_SHA256}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(wxWidgets)
    set(RAY_WX_LIBRARIES wx::gl wx::propgrid wx::core wx::base)
endif()

set(RAY_EDITOR_SOURCES
    ${PROJECT_SOURCE_DIR}/editor/gui/EditorApp.cpp
    ${PROJECT_SOURCE_DIR}/editor/gui/MainFrame.cpp
    ${PROJECT_SOURCE_DIR}/editor/gui/PropertyPanel.cpp
    ${PROJECT_SOURCE_DIR}/editor/gui/SceneCanvas.cpp
)
if(WIN32)
    list(APPEND RAY_EDITOR_SOURCES ${PROJECT_SOURCE_DIR}/editor/gui/RayEditor.rc)
endif()

add_executable(ray-editor WIN32 MACOSX_BUNDLE ${RAY_EDITOR_SOURCES})
target_include_directories(ray-editor PRIVATE ${PROJECT_SOURCE_DIR}/editor/gui)
target_link_libraries(ray-editor PRIVATE ray_editor_logic ray_core ${RAY_WX_LIBRARIES} OpenGL::GL)
set_target_properties(ray-editor PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Ray Editor"
    MACOSX_BUNDLE_GUI_IDENTIFIER "net.naujok.ray.editor")
if(MSVC)
    target_compile_options(ray-editor PRIVATE /W3)
else()
    target_compile_options(ray-editor PRIVATE -Wall)
endif()
if(APPLE)
    # The view's OpenGL 1.x calls are deprecated on macOS, but still there.
    target_compile_definitions(ray-editor PRIVATE GL_SILENCE_DEPRECATION)
endif()
install(TARGETS ray-editor RUNTIME DESTINATION bin COMPONENT ray BUNDLE DESTINATION . COMPONENT ray)
