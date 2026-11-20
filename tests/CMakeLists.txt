# Regression tests: every readable sample scene is rendered and compared with
# a reference render (some again with anti-aliasing or other color models); the TCP engines must produce exactly what the threads
# do, and scene files must survive a load and save.

add_executable(compare_images compare_images.cpp)

add_executable(format_test format_test.cpp)
target_link_libraries(format_test PRIVATE ray_core)

set(REFERENCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/reference")
file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/reference/renders.tar.gz"
                     DESTINATION "${REFERENCE_DIR}")
set(RENDER_DIR "${CMAKE_CURRENT_BINARY_DIR}/renders")
file(MAKE_DIRECTORY "${RENDER_DIR}")

set(RAY_TEST_SCENES
    axis_test huh inside_sphere mat_test reflect_me reflect_test2 reflect_test3
    reflection_test scan_test ser_test ser_test2 spec_test test triangle triangle2 xxx)

# The scenes also rendered by networked engines.
set(RAY_NETWORK_SCENES scan_test triangle reflect_test3 xxx)

# The scenes also rendered with corner anti-aliasing (--aa corners), and the
# ones of those also rendered by networked engines.
set(RAY_AA_SCENES scan_test axis_test triangle xxx)
set(RAY_AA_NETWORK_SCENES scan_test xxx)

# The other samplers, rendered with scan_test: name, then the --aa value.
set(RAY_SAMPLER_TESTS
    box       box
    random2   random:samples=2
    random4   random:samples=4
    random9   random:samples=9)
set(RAY_SAMPLER_SCENE scan_test)

# The color models other than the default, each rendered with one scene.
set(RAY_COLOR_MODELS gamma srgb reinhard)
set(RAY_COLOR_SCENE scan_test)

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

list(LENGTH RAY_SAMPLER_TESTS count)
math(EXPR last "${count} - 1")
foreach(index RANGE 0 ${last} 2)
    math(EXPR value_index "${index} + 1")
    list(GET RAY_SAMPLER_TESTS ${index} name)
    list(GET RAY_SAMPLER_TESTS ${value_index} sampler)
    add_test(NAME render_sampler_${name}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${RAY_SAMPLER_SCENE}.ray
                     -DOUTPUT=${RENDER_DIR}/${RAY_SAMPLER_SCENE}_${name}.raw
                     -DREFERENCE=${REFERENCE_DIR}/${RAY_SAMPLER_SCENE}_${name}.raw
                     -DAA=${sampler}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    set_tests_properties(render_sampler_${name} PROPERTIES FIXTURES_SETUP render_sampler_${name} TIMEOUT 300)
endforeach()

# The Utah teapot: a smooth, double-sided mesh of 6,320 triangles. Rendered
# small, since every ray inside its bounding box tests every face.
add_test(NAME render_teapot
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/teapot.ray
                 -DOUTPUT=${RENDER_DIR}/teapot.raw
                 -DREFERENCE=${REFERENCE_DIR}/teapot.raw
                 -DSIZE=160x120
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_teapot PROPERTIES TIMEOUT 600)
# A real lens: scan_test through the double Gauss, focused on the glass
# sphere, with a stratified sampler. Small, since it traces 25 rays a pixel.
add_test(NAME render_lens
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/lens_test.ray
                 -DOUTPUT=${RENDER_DIR}/lens_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/lens_test.raw
                 -DSIZE=160x120
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_lens PROPERTIES TIMEOUT 600)
add_test(NAME roundtrip_lens_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/lens_test.ray ${RENDER_DIR})
# Chromatic aberration: scan_test through an uncorrected BK7 singlet, each
# sample traced in red, green and blue apart.
add_test(NAME render_chromatic
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/chromatic_test.ray
                 -DOUTPUT=${RENDER_DIR}/chromatic_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/chromatic_test.raw
                 -DSIZE=160x120
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_chromatic PROPERTIES FIXTURES_SETUP render_chromatic TIMEOUT 600)
add_test(NAME roundtrip_chromatic_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/chromatic_test.ray ${RENDER_DIR})

# Textures: a checkered floor, an image on a wall and a sphere, and a solid
# checker. The image's pixels must reach networked engines in the scene.
add_test(NAME render_texture
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/texture_test.ray
                 -DOUTPUT=${RENDER_DIR}/texture_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/texture_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_texture PROPERTIES FIXTURES_SETUP render_texture TIMEOUT 300)
add_test(NAME roundtrip_texture_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/texture_test.ray ${RENDER_DIR})
add_test(NAME render_teapot_textured
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/teapot_textured.ray
                 -DOUTPUT=${RENDER_DIR}/teapot_textured.raw
                 -DREFERENCE=${REFERENCE_DIR}/teapot_textured.raw
                 -DSIZE=120x90
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_teapot_textured PROPERTIES TIMEOUT 600)
add_test(NAME roundtrip_teapot_textured COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/teapot_textured.ray ${RENDER_DIR})
add_test(NAME texture_checks COMMAND format_test --textures ${PROJECT_SOURCE_DIR}/scenes)

add_test(NAME render_sampler_stratified
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${RAY_SAMPLER_SCENE}.ray
                 -DOUTPUT=${RENDER_DIR}/${RAY_SAMPLER_SCENE}_stratified.raw
                 -DREFERENCE=${REFERENCE_DIR}/${RAY_SAMPLER_SCENE}_stratified.raw
                 -DAA=stratified:grid=3
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)

add_test(NAME roundtrip_teapot COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/teapot.ray ${RENDER_DIR})

foreach(model IN LISTS RAY_COLOR_MODELS)
    add_test(NAME render_color_${model}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${RAY_COLOR_SCENE}.ray
                     -DOUTPUT=${RENDER_DIR}/${RAY_COLOR_SCENE}_${model}.raw
                     -DREFERENCE=${REFERENCE_DIR}/${RAY_COLOR_SCENE}_${model}.raw
                     -DCOLOR=${model}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
endforeach()

foreach(scene IN LISTS RAY_AA_SCENES)
    add_test(NAME render_aa_${scene}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${scene}.ray
                     -DOUTPUT=${RENDER_DIR}/${scene}_aa.raw
                     -DREFERENCE=${REFERENCE_DIR}/${scene}_aa.raw
                     -DAA=corners
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    set_tests_properties(render_aa_${scene} PROPERTIES FIXTURES_SETUP render_aa_${scene} TIMEOUT 300)
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

foreach(scene IN LISTS RAY_AA_NETWORK_SCENES)
    math(EXPR port "${port} + 1")
    add_test(NAME network_aa_${scene}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${scene}.ray
                     -DOUTPUT=${RENDER_DIR}/${scene}_aa_network.raw
                     -DMODE=network
                     -DPORT=${port}
                     -DAA=corners
                     -DEXACT_WITH=${RENDER_DIR}/${scene}_aa.raw
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    set_tests_properties(network_aa_${scene} PROPERTIES FIXTURES_REQUIRED render_aa_${scene}
                                                     RESOURCE_LOCK ray_network TIMEOUT 300)
endforeach()

# The random sampler's points must not depend on which task traces a ray.
math(EXPR port "${port} + 1")
add_test(NAME network_sampler_random4
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${RAY_SAMPLER_SCENE}.ray
                 -DOUTPUT=${RENDER_DIR}/${RAY_SAMPLER_SCENE}_random4_network.raw
                 -DMODE=network
                 -DPORT=${port}
                 -DAA=random:samples=4
                 -DEXACT_WITH=${RENDER_DIR}/${RAY_SAMPLER_SCENE}_random4.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(network_sampler_random4 PROPERTIES FIXTURES_REQUIRED render_sampler_random4
                                                        RESOURCE_LOCK ray_network TIMEOUT 300)

# The channel a ray is traced in travels in its tag; it must come back from
# networked engines intact.
math(EXPR port "${port} + 1")
add_test(NAME network_chromatic
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/chromatic_test.ray
                 -DOUTPUT=${RENDER_DIR}/chromatic_test_network.raw
                 -DMODE=network
                 -DPORT=${port}
                 -DSIZE=160x120
                 -DEXACT_WITH=${RENDER_DIR}/chromatic_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(network_chromatic PROPERTIES FIXTURES_REQUIRED render_chromatic
                                                  RESOURCE_LOCK ray_network TIMEOUT 600)

math(EXPR port "${port} + 1")
add_test(NAME network_texture
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/texture_test.ray
                 -DOUTPUT=${RENDER_DIR}/texture_test_network.raw
                 -DMODE=network
                 -DPORT=${port}
                 -DEXACT_WITH=${RENDER_DIR}/texture_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(network_texture PROPERTIES FIXTURES_REQUIRED render_texture
                                                RESOURCE_LOCK ray_network TIMEOUT 300)

# raythrower --spawn starts the rayengine next to it.
add_dependencies(raythrower rayengine)
