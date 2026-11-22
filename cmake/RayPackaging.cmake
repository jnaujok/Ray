# Installing Ray, and packaging it as .deb and .rpm with CPack (Linux).
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRAY_BUILD_EDITOR=ON -DCMAKE_INSTALL_PREFIX=/usr \
#         -DRAY_PACKAGE_VERSION=0.21.2 -DRAY_PACKAGE_STAGE=alpha
#   cmake --build build --parallel
#   cd build && cpack -G DEB     # or RPM
#
# Everything Ray installs is in the "ray" component, so the packages leave out whatever IASLib's
# and wxWidgets' own builds would install. The release workflow (.github/workflows/release.yml)
# works out the version from the tag (2026-alpha-0.21.2 is version 0.21.2, stage alpha).

include(GNUInstallDirs)

set(RAY_PACKAGE_VERSION "0.0.0" CACHE STRING "The version the packages carry (from the release tag)")
set(RAY_PACKAGE_STAGE "" CACHE STRING "A pre-release stage for the packages (alpha, beta), or empty")

install(TARGETS raythrower rayengine RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ray)

if(UNIX AND NOT APPLE)
    # The sample scenes, with their models, fonts and textures, so they render as they are.
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/scenes/ DESTINATION ${CMAKE_INSTALL_DATADIR}/ray/scenes COMPONENT ray)
    install(FILES ${PROJECT_SOURCE_DIR}/LICENSE ${PROJECT_SOURCE_DIR}/NOTICE ${PROJECT_SOURCE_DIR}/README.md
            DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/ray COMPONENT ray)
    # Debian looks for the license as "copyright", and for a changelog.Debian.gz.
    install(FILES ${PROJECT_SOURCE_DIR}/packaging/linux/copyright
            DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/ray COMPONENT ray)
    if(RAY_PACKAGE_STAGE)
        set(RAY_DEBIAN_VERSION "${RAY_PACKAGE_VERSION}~${RAY_PACKAGE_STAGE}-1")
    else()
        set(RAY_DEBIAN_VERSION "${RAY_PACKAGE_VERSION}-1")
    endif()
    string(TIMESTAMP RAY_DEBIAN_DATE "%a, %d %b %Y %H:%M:%S +0000" UTC)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian
         "ray (${RAY_DEBIAN_VERSION}) unstable; urgency=low\n\n"
         "  * Ray ${RAY_PACKAGE_VERSION} ${RAY_PACKAGE_STAGE}; see /usr/share/doc/ray/changelog.\n\n"
         " -- Jeffrey R. Naujok <jnaujok@users.noreply.github.com>  ${RAY_DEBIAN_DATE}\n")
    find_program(RAY_GZIP gzip)
    if(RAY_GZIP)
            # The changelog, compressed as Debian expects (a change to it
            # configures again, through configure_file).
        configure_file(${PROJECT_SOURCE_DIR}/CHANGELOG.md ${CMAKE_CURRENT_BINARY_DIR}/changelog COPYONLY)
        execute_process(COMMAND ${RAY_GZIP} -9 -n -f ${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian)
        execute_process(COMMAND ${RAY_GZIP} -9 -n -f -k ${CMAKE_CURRENT_BINARY_DIR}/changelog)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian.gz ${CMAKE_CURRENT_BINARY_DIR}/changelog.gz
                DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/ray COMPONENT ray)
    else()
        install(FILES ${PROJECT_SOURCE_DIR}/CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/ray COMPONENT ray)
    endif()
    if(RAY_BUILD_EDITOR)
        install(FILES ${PROJECT_SOURCE_DIR}/packaging/io.github.jnaujok.Ray.desktop
                DESTINATION ${CMAKE_INSTALL_DATADIR}/applications COMPONENT ray)
        install(FILES ${PROJECT_SOURCE_DIR}/packaging/io.github.jnaujok.Ray.metainfo.xml
                DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo COMPONENT ray)
        install(FILES ${PROJECT_SOURCE_DIR}/packaging/io.github.jnaujok.Ray.svg
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps COMPONENT ray)
        install(FILES ${PROJECT_SOURCE_DIR}/packaging/io.github.jnaujok.Ray.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps COMPONENT ray)
    endif()
endif()

set(CPACK_PACKAGE_NAME "ray")
set(CPACK_PACKAGE_VENDOR "Jeffrey R. Naujok")
set(CPACK_PACKAGE_CONTACT "Jeffrey R. Naujok <jnaujok@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "distributed ray tracer from 1997, rebuilt")
set(CPACK_PACKAGE_DESCRIPTION
    "Ray splits ray tracing across a network of machines. raythrower renders\nscenes from the command line, with threads or with rayengine processes on\nother machines; ray-editor edits scenes in a shaded OpenGL view and\nray-traces them in place. It traces real camera lenses, spectral\ndispersion, area lights, bounce light, haze, solid textures, and meshes\nfrom STL, OBJ, glTF and Blender files.")
set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/jnaujok/Ray")
set(CPACK_PACKAGE_VERSION "${RAY_PACKAGE_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_COMPONENTS_ALL ray)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_STRIP_FILES ON)

# Debian: 0.21.2~alpha sorts before 0.21.2, as a pre-release should.
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_NAME "ray")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
if(RAY_PACKAGE_STAGE)
    set(CPACK_DEBIAN_PACKAGE_VERSION "${RAY_PACKAGE_VERSION}~${RAY_PACKAGE_STAGE}")
else()
    set(CPACK_DEBIAN_PACKAGE_VERSION "${RAY_PACKAGE_VERSION}")
endif()
set(CPACK_DEBIAN_PACKAGE_RELEASE "1")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
if(RAY_BUILD_EDITOR)
    # dpkg-shlibdeps finds the C and C++ runtime; GTK and OpenGL are named too, in case it can't.
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtk-3-0t64 | libgtk-3-0, libgl1, libglu1-mesa")
endif()

# RPM: version 0.21.2, release 0.alpha (a release below 1, so the final 0.21.2-1 replaces it).
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_PACKAGE_NAME "ray")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_VERSION "${RAY_PACKAGE_VERSION}")
if(RAY_PACKAGE_STAGE)
    set(CPACK_RPM_PACKAGE_RELEASE "0.${RAY_PACKAGE_STAGE}")
else()
    set(CPACK_RPM_PACKAGE_RELEASE "1")
endif()
set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Graphics")
set(CPACK_RPM_PACKAGE_URL "https://github.com/jnaujok/Ray")
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
if(RAY_BUILD_EDITOR)
    set(CPACK_RPM_PACKAGE_REQUIRES "gtk3, mesa-libGL, mesa-libGLU")
endif()
# Directories the system already owns.
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr/share/applications /usr/share/metainfo /usr/share/icons /usr/share/icons/hicolor
    /usr/share/icons/hicolor/scalable /usr/share/icons/hicolor/scalable/apps
    /usr/share/icons/hicolor/256x256 /usr/share/icons/hicolor/256x256/apps /usr/share/doc)

include(CPack)
