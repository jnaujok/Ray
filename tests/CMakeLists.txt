# Regression tests: every readable sample scene is rendered and compared with
# a reference render, the TCP engines must produce exactly what the threads
# do, and scene files must survive a load and save.

add_executable(compare_images compare_images.cpp)

add_executable(format_test format_test.cpp)
target_link_libraries(format_test PRIVATE ray_core)

set(REFERENCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/reference")
file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/reference/renders.tar.gz"
                     DESTINATION "${REFERENCE_DIR}")
set(RENDER_DIR "${CMAKE_CURRENT_BINARY_DIR}/renders")
file(MAKE_DIRECTORY "${RENDER_DIR}")

# stream_test1.ray and yyy.ray are development artifacts that no version of
# the program could read (see scenes/README.md).
set(RAY_TEST_SCENES
    axis_test huh inside_sphere mat_test reflect_me reflect_test2 reflect_test3
    reflection_test scan_test ser_test ser_test2 spec_test test triangle triangle2 xxx)

# The scenes also rendered by networked engines.
set(RAY_NETWORK_SCENES scan_test triangle reflect_test3 xxx)

add_test(NAME ray_stream_format COMMAND format_test)

foreach(scene IN LISTS RAY_TEST_SCENES)
    set(scene_file "${PROJECT_SOURCE_DIR}/scenes/${scene}.ray")

    add_test(NAME render_${scene}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${scene_file}
                     -DOUTPUT=${RENDER_DIR}/${scene}.raw
                     -DREFERENCE=${REFERENCE_DIR}/${scene}.raw
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    set_tests_properties(render_${scene} PROPERTIES FIXTURES_SETUP render_${scene} TIMEOUT 300)

    add_test(NAME roundtrip_${scene} COMMAND format_test ${scene_file} ${RENDER_DIR})
endforeach()

# Each networked test listens on its own port, outside the range the system
# hands out for outgoing connections, and they run one at a time.
set(port 27997)
foreach(scene IN LISTS RAY_NETWORK_SCENES)
    math(EXPR port "${port} + 1")
    add_test(NAME network_${scene}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${scene}.ray
                     -DOUTPUT=${RENDER_DIR}/${scene}_network.raw
                     -DMODE=network
                     -DPORT=${port}
                     -DEXACT_WITH=${RENDER_DIR}/${scene}.raw
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    set_tests_properties(network_${scene} PROPERTIES FIXTURES_REQUIRED render_${scene}
                                                     RESOURCE_LOCK ray_network TIMEOUT 300)
endforeach()

# raythrower --spawn starts the rayengine next to it.
add_dependencies(raythrower rayengine)
