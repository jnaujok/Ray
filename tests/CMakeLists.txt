# Regression tests: every readable sample scene is rendered and compared with
# a reference render (some again with anti-aliasing or other color models); the TCP engines must produce exactly what the threads
# do, and scene files must survive a load and save.

add_executable(compare_images compare_images.cpp)

add_executable(format_test format_test.cpp)
target_link_libraries(format_test PRIVATE ray_core)

# The scene editor's logic, without a display.
add_executable(editor_test editor_test.cpp)
target_link_libraries(editor_test PRIVATE ray_editor_logic ray_core)

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
    random9   random:samples=9
    adaptive  adaptive)
set(RAY_SAMPLER_SCENE scan_test)

# The color models other than the default, each rendered with one scene.
set(RAY_COLOR_MODELS gamma srgb reinhard hdr)
set(RAY_COLOR_SCENE scan_test)

add_test(NAME ray_stream_format COMMAND format_test)

# Windows headers make min and max macros; this catches calls that would break
# the Windows build, on every platform.
add_test(NAME lint_minmax
         COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/LintMinMax.cmake)

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
# small, as it was before meshes had a bounding volume hierarchy.
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
# sample traced at three single wavelengths (spectral dispersion).
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
add_test(NAME texture_checks COMMAND format_test --textures ${PROJECT_SOURCE_DIR}/scenes ${RENDER_DIR})
# Engines that come and go: one quits after its first tile, another joins
# during the trace; the image must still be exactly what threads make.
add_test(NAME engine_churn COMMAND format_test --engines ${PROJECT_SOURCE_DIR}/scenes ${RENDER_DIR})
set_tests_properties(engine_churn PROPERTIES RESOURCE_LOCK ray_network TIMEOUT 120)

# Cylinders, cones and boxes: open and capped, truncated, turned, textured
# and glass.
add_test(NAME render_primitives
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/primitives_test.ray
                 -DOUTPUT=${RENDER_DIR}/primitives_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/primitives_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_primitives PROPERTIES FIXTURES_SETUP render_primitives TIMEOUT 300)
add_test(NAME roundtrip_primitives_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/primitives_test.ray ${RENDER_DIR})

# Solid textures (marble, wood, noise) and glass that absorbs by the
# distance light goes through it.
add_test(NAME render_solid_textures
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/solid_textures_test.ray
                 -DOUTPUT=${RENDER_DIR}/solid_textures_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/solid_textures_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_solid_textures PROPERTIES FIXTURES_SETUP render_solid_textures TIMEOUT 300)
add_test(NAME roundtrip_solid_textures_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/solid_textures_test.ray ${RENDER_DIR})

# Rough (frosted) glass, a brushed mirror, and soft shadows from a
# rectangle light and a sphere light, with a 6x6 stratified sampler.
add_test(NAME render_soft
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/soft_test.ray
                 -DOUTPUT=${RENDER_DIR}/soft_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/soft_test.raw
                 -DSIZE=160x120
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_soft PROPERTIES FIXTURES_SETUP render_soft TIMEOUT 900)
add_test(NAME roundtrip_soft_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/soft_test.ray ${RENDER_DIR})

# Bump and normal maps, and PNG textures: a brick wall (a PNG color and a
# PNG normal map), hammered copper and orange peel (noise height maps), a
# box embossed by a 16-bit PNG height map, and a wood floor bumped by its
# own grain.
add_test(NAME render_bump
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/bump_test.ray
                 -DOUTPUT=${RENDER_DIR}/bump_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/bump_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_bump PROPERTIES FIXTURES_SETUP render_bump TIMEOUT 300)
add_test(NAME roundtrip_bump_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/bump_test.ray ${RENDER_DIR})

# Spot lights (a hard edge, a soft edge, and a soft-edged disc with soft
# shadows) and a sun with an angular size, whose shadows blur with distance.
add_test(NAME render_lights
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/lights_test.ray
                 -DOUTPUT=${RENDER_DIR}/lights_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/lights_test.raw
                 -DSIZE=160x120
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_lights PROPERTIES FIXTURES_SETUP render_lights TIMEOUT 300)
add_test(NAME roundtrip_lights_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/lights_test.ray ${RENDER_DIR})

# Light bounced between surfaces: a box with a red wall and a green one,
# whose colors bleed onto the white walls, floor, block and ball, and fill
# light in the shadows and on the ceiling.
add_test(NAME render_bounce
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/bounce_test.ray
                 -DOUTPUT=${RENDER_DIR}/bounce_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/bounce_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_bounce PROPERTIES FIXTURES_SETUP render_bounce TIMEOUT 300)
add_test(NAME roundtrip_bounce_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/bounce_test.ray ${RENDER_DIR})

# The same room with the "hemisphere" method, at the same rays a point (54:
# 9 an object for the 6 objects in reach), for contrast.
add_test(NAME render_bounce_hemisphere
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/bounce_hemisphere_test.ray
                 -DOUTPUT=${RENDER_DIR}/bounce_hemisphere_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/bounce_hemisphere_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_bounce_hemisphere PROPERTIES TIMEOUT 300)
add_test(NAME roundtrip_bounce_hemisphere_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/bounce_hemisphere_test.ray ${RENDER_DIR})

# Haze: an avenue of columns fading into ground mist under a sky.
add_test(NAME render_haze
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/haze_test.ray
                 -DOUTPUT=${RENDER_DIR}/haze_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/haze_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_haze PROPERTIES FIXTURES_SETUP render_haze TIMEOUT 300)
add_test(NAME roundtrip_haze_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/haze_test.ray ${RENDER_DIR})

# The material presets: thirty spheres, one of each finish in the library.
add_test(NAME render_materials
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/materials_test.ray
                 -DOUTPUT=${RENDER_DIR}/materials_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/materials_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_materials PROPERTIES FIXTURES_SETUP render_materials TIMEOUT 300)
add_test(NAME roundtrip_materials_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/materials_test.ray ${RENDER_DIR})

# Beam profiles on a wall, a slide projected from a spot, an attenuated light.
add_test(NAME render_projector
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/projector_test.ray
                 -DOUTPUT=${RENDER_DIR}/projector_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/projector_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_projector PROPERTIES FIXTURES_SETUP render_projector TIMEOUT 300)
add_test(NAME roundtrip_projector_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/projector_test.ray ${RENDER_DIR})

# Underwater: blue-green haze, sand, and caustics projected from the sun.
add_test(NAME render_underwater
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/underwater_test.ray
                 -DOUTPUT=${RENDER_DIR}/underwater_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/underwater_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_underwater PROPERTIES FIXTURES_SETUP render_underwater TIMEOUT 300)
add_test(NAME roundtrip_underwater_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/underwater_test.ray ${RENDER_DIR})

# Texture maps: frosting, polish, index and see-through patches from
# textures, bumps of varying depth, and a light colored by a 3D texture.
add_test(NAME render_maps
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/maps_test.ray
                 -DOUTPUT=${RENDER_DIR}/maps_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/maps_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_maps PROPERTIES FIXTURES_SETUP render_maps TIMEOUT 300)
add_test(NAME roundtrip_maps_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/maps_test.ray ${RENDER_DIR})

# Textures inside glass: art glass swirls, smoky quartz, amber with dark
# flecks, and veins floating in a clear block.
add_test(NAME render_volume
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/volume_test.ray
                 -DOUTPUT=${RENDER_DIR}/volume_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/volume_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_volume PROPERTIES FIXTURES_SETUP render_volume TIMEOUT 300)
add_test(NAME roundtrip_volume_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/volume_test.ray ${RENDER_DIR})

# The shapes of 0.16: a torus, a disc and an annulus, an open paraboloid, a
# capped hyperboloid, a rounded and a pinched superquadric, and a rounded box.
add_test(NAME render_shapes
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/shapes_test.ray
                 -DOUTPUT=${RENDER_DIR}/shapes_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/shapes_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_shapes PROPERTIES FIXTURES_SETUP render_shapes TIMEOUT 300)
add_test(NAME roundtrip_shapes_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/shapes_test.ray ${RENDER_DIR})

# Text from a TrueType font: beveled gold letters, two lines set centered,
# and a word carved out of a rounded slab with a csg.
add_test(NAME render_text
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/text_test.ray
                 -DOUTPUT=${RENDER_DIR}/text_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/text_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_text PROPERTIES FIXTURES_SETUP render_text TIMEOUT 300)
add_test(NAME roundtrip_text_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/text_test.ray ${RENDER_DIR})

# Bezier patches: the Utah teapot as its 32 patches beside the mesh teapot.
# NURBS: a rational torus beside the analytic one, and a B-spline sheet with
# a texture and a bump map across it. Lathes: chess pieces, a glass goblet, an
# open vase and a cut-away orange. Extrusions: a tapered star, a twisted
# column, a beveled frame with a hole, and sweeps (a spring, a closed loop and
# a square rail).
foreach(scene IN ITEMS patches nurbs lathe extrusion implicit blob fractal)
    add_test(NAME render_${scene}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${scene}_test.ray
                     -DOUTPUT=${RENDER_DIR}/${scene}_test.raw
                     -DREFERENCE=${REFERENCE_DIR}/${scene}_test.raw
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    set_tests_properties(render_${scene} PROPERTIES FIXTURES_SETUP render_${scene} TIMEOUT 300)
    add_test(NAME roundtrip_${scene}_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/${scene}_test.ray ${RENDER_DIR})
endforeach()

# The Apollo cameras: the Hasselblad Electric Camera with its 80 mm Planar,
# Kodak film (its curve, grain and halation), the Data Camera's Reseau
# crosses blooming over by a bright sphere, and Tranquility Base on regolith;
# the Najdorf chessboard through the double Gauss wide open (glTF pieces), and
# NASA frame AS11-40-5903 recreated (glTF astronaut and lunar module, a gold visor).
foreach(scene IN ITEMS apollo_lenses film reseau tranquility chess_dof as11_40_5903)
    add_test(NAME render_${scene}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${scene}_test.ray
                     -DOUTPUT=${RENDER_DIR}/${scene}_test.raw
                     -DREFERENCE=${REFERENCE_DIR}/${scene}_test.raw
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    set_tests_properties(render_${scene} PROPERTIES FIXTURES_SETUP render_${scene} TIMEOUT 300)
    add_test(NAME roundtrip_${scene}_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/${scene}_test.ray ${RENDER_DIR})
endforeach()

# An environment map: an HDR sky behind the scene, in a mirror ball and
# through a glass one, lighting the diffuse surfaces (image-based lighting).
add_test(NAME render_environment
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/environment_test.ray
                 -DOUTPUT=${RENDER_DIR}/environment_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/environment_test.raw
                 -DSIZE=160x120
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_environment PROPERTIES FIXTURES_SETUP render_environment TIMEOUT 300)
add_test(NAME roundtrip_environment_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/environment_test.ray ${RENDER_DIR})

# Constructive solid geometry: a lens blank (two spheres' intersection), a
# bowl (a shell cut by a turned box, shaded by its parts' materials), a die
# (a rounded cube, itself a csg, with drilled pips painted black) and a
# hammered capsule (a union).
add_test(NAME render_csg
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/csg_test.ray
                 -DOUTPUT=${RENDER_DIR}/csg_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/csg_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_csg PROPERTIES FIXTURES_SETUP render_csg TIMEOUT 300)
add_test(NAME roundtrip_csg_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/csg_test.ray ${RENDER_DIR})

# Dispersion: a flint glass prism (a csg) and ball before white bars, which
# they fan out into a spectrum (spectral dispersion).
add_test(NAME render_prism
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/prism_test.ray
                 -DOUTPUT=${RENDER_DIR}/prism_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/prism_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_prism PROPERTIES FIXTURES_SETUP render_prism TIMEOUT 900 LABELS slow)
add_test(NAME roundtrip_prism_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/prism_test.ray ${RENDER_DIR})

# Model files: a puzzle cube and a gem from colored STL files (the VisCAM and
# Magics conventions) and a ring from an OBJ file with texture coordinates.
add_test(NAME render_models
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/models_test.ray
                 -DOUTPUT=${RENDER_DIR}/models_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/models_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_models PROPERTIES FIXTURES_SETUP render_models TIMEOUT 300)
add_test(NAME roundtrip_models_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/models_test.ray ${RENDER_DIR})

# 3DBenchy (225,706 triangles from its STL file) printed in orange plastic,
# close up, its layer lines a solid texture as both color and bump map.
add_test(NAME render_benchy
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/benchy_test.ray
                 -DOUTPUT=${RENDER_DIR}/benchy_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/benchy_test.raw
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
set_tests_properties(render_benchy PROPERTIES FIXTURES_SETUP render_benchy TIMEOUT 900 LABELS slow)
add_test(NAME roundtrip_benchy_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/benchy_test.ray ${RENDER_DIR})

# Tests labelled "slow" take minutes (more on a small machine or under the
# sanitizers); `ctest -LE slow` runs the rest, fixtures and all.

# The torture test: every kind of object (the teapot too), reflection,
# refraction and textures, through the double Gauss with a 5x5 stratified
# sampler. Rendered small; the scene itself is 1024x768.
add_test(NAME render_torture
         COMMAND ${CMAKE_COMMAND}
                 -DRAYTHROWER=$<TARGET_FILE:raythrower>
                 -DCOMPARE=$<TARGET_FILE:compare_images>
                 -DSCENE=${PROJECT_SOURCE_DIR}/scenes/torture_test.ray
                 -DOUTPUT=${RENDER_DIR}/torture_test.raw
                 -DREFERENCE=${REFERENCE_DIR}/torture_test.raw
                 -DSIZE=160x120
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
# Labelled torture as well as slow: CI leaves it out (ctest -LE torture), since the runners are
# too slow for it; the full preset runs it locally before every merge.
set_tests_properties(render_torture PROPERTIES TIMEOUT 900 LABELS "slow;torture")
add_test(NAME roundtrip_torture_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/torture_test.ray ${RENDER_DIR})

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

# Networked engines must produce exactly what threads do, byte for byte.
# That holds at any size, so each networked test renders its scene small
# (RAY_NETWORK_SIZE, in RAY_NETWORK_TILE tiles, so three engines still share
# many tiles), once with threads and once with engines, and compares the
# two; it needs no other test. Each listens on its own port, outside the
# range the system hands out for outgoing connections, so they can run at
# the same time.
set(RAY_NETWORK_SIZE 96x72)
set(RAY_NETWORK_TILE 16)
set(ray_network_port 27997)

# ray_network_test(<name> <scene file> [AA <sampler>] [LABELS <labels>])
function(ray_network_test name scene)
    cmake_parse_arguments(NET "" "AA;LABELS" "" ${ARGN})
    math(EXPR port "${ray_network_port} + 1")
    set(ray_network_port ${port} PARENT_SCOPE)
    set(aa_arg "")
    if(NET_AA)
        set(aa_arg "-DAA=${NET_AA}")
    endif()
    add_test(NAME network_${name}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${scene}
                     -DOUTPUT=${RENDER_DIR}/${name}_network.raw
                     -DMODE=network
                     -DSELF_CHECK=ON
                     -DPORT=${port}
                     -DSIZE=${RAY_NETWORK_SIZE}
                     -DTILE=${RAY_NETWORK_TILE}
                     ${aa_arg}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    # A master and three engines.
    set_tests_properties(network_${name} PROPERTIES PROCESSORS 4 TIMEOUT 300)
    if(NET_LABELS)
        set_tests_properties(network_${name} PROPERTIES LABELS ${NET_LABELS})
    endif()
endfunction()

foreach(scene IN LISTS RAY_NETWORK_SCENES)
    ray_network_test(${scene} ${PROJECT_SOURCE_DIR}/scenes/${scene}.ray)
endforeach()
foreach(scene IN LISTS RAY_AA_NETWORK_SCENES)
    ray_network_test(aa_${scene} ${PROJECT_SOURCE_DIR}/scenes/${scene}.ray AA corners)
endforeach()

# The rays a render casts, by kind (raythrower --stats-json), must be the same
# with 4 threads, 2 threads and 3 networked engines. Between them the scenes
# cast every kind: area, spot, sun and sky shadow rays, bounce rays,
# reflections and refractions.
foreach(scene IN ITEMS soft lights environment bounce)
    math(EXPR port "${ray_network_port} + 1")
    set(ray_network_port ${port})
    add_test(NAME ray_counts_${scene}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${scene}_test.ray
                     -DOUTPUT=${RENDER_DIR}
                     -DSIZE=48x36
                     -DPORT=${port}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RayCountTest.cmake)
    set_tests_properties(ray_counts_${scene} PROPERTIES PROCESSORS 4 TIMEOUT 300)
endforeach()

# The random sampler's points must not depend on which task traces a ray, and
# the adaptive sampler decides on rays from colors; engines must decide the same.
ray_network_test(sampler_random4 ${PROJECT_SOURCE_DIR}/scenes/${RAY_SAMPLER_SCENE}.ray AA random:samples=4)
ray_network_test(sampler_adaptive ${PROJECT_SOURCE_DIR}/scenes/${RAY_SAMPLER_SCENE}.ray AA adaptive)

# Models read straight from Blender .blend files, in both of Blender's mesh
# layouts, Zstandard- and gzip-compressed; and textures from JPEG and WebP
# files beside their PNG original; and a lens built as glass with its ray
# fans (lensElements).
foreach(scene IN ITEMS blend images lens_cutaway)
    add_test(NAME render_${scene}
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DCOMPARE=$<TARGET_FILE:compare_images>
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/${scene}_test.ray
                     -DOUTPUT=${RENDER_DIR}/${scene}_test.raw
                     -DREFERENCE=${REFERENCE_DIR}/${scene}_test.raw
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/RenderTest.cmake)
    set_tests_properties(render_${scene} PROPERTIES FIXTURES_SETUP render_${scene} TIMEOUT 300)
    add_test(NAME roundtrip_${scene}_test COMMAND format_test ${PROJECT_SOURCE_DIR}/scenes/${scene}_test.ray ${RENDER_DIR})
endforeach()

# The channel a ray is traced in travels in its tag; it must come back from
# networked engines intact. Engines get texture images, PNGs, the sky and
# model files inside the scene, so they need no files.
foreach(scene IN ITEMS chromatic texture primitives solid_textures soft bump lights bounce bounce_hemisphere haze
                       materials projector underwater environment csg prism models benchy maps volume shapes text
                       patches nurbs lathe extrusion implicit blob fractal apollo_lenses film reseau tranquility
                       chess_dof as11_40_5903 blend images lens_cutaway)
    ray_network_test(${scene} ${PROJECT_SOURCE_DIR}/scenes/${scene}_test.ray)
endforeach()

# PNG and Radiance HDR output, checked with Python's own zlib.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME image_output
             COMMAND ${CMAKE_COMMAND}
                     -DRAYTHROWER=$<TARGET_FILE:raythrower>
                     -DPYTHON=${Python3_EXECUTABLE}
                     -DCHECK=${PROJECT_SOURCE_DIR}/tools/check_image.py
                     -DSCENE=${PROJECT_SOURCE_DIR}/scenes/scan_test.ray
                     -DOUTPUT=${RENDER_DIR}/image_output
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/ImageOutputTest.cmake)
    set_tests_properties(image_output PROPERTIES TIMEOUT 300)
else()
    message(STATUS "Python 3 not found: the image_output test is skipped")
endif()

# The scene editor's logic: editing, undo, tessellation, the view's camera
# against the tracer's, picking, model import, saving and the progressive
# render (no display needed).
add_test(NAME editor_logic COMMAND editor_test ${PROJECT_SOURCE_DIR}/scenes ${RENDER_DIR})
set_tests_properties(editor_logic PROPERTIES TIMEOUT 300)

# raythrower --spawn starts the rayengine next to it.
add_dependencies(raythrower rayengine)
