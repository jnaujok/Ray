# Renders a scene with raythrower and compares the result.
#
#   cmake -DRAYTHROWER=... -DCOMPARE=... -DSCENE=... -DOUTPUT=...
#         [-DREFERENCE=...] [-DMODE=threads|network] [-DEXACT_WITH=...]
#         [-DSELF_CHECK=ON] [-DAA=<sampler>] [-DCOLOR=<color model>]
#         [-DSIZE=<w>x<h>] [-DTILE=<pixels>] -P RenderTest.cmake
#
# MODE network renders with rayengine processes over TCP. EXACT_WITH names
# another render the output must match byte for byte; SELF_CHECK renders the
# scene with threads first, with the same options, and makes that the one to
# match. AA and COLOR pick the sampler and color model instead of the
# scene's; TILE the size of the tiles the tasks ask for.

if(NOT MODE)
    set(MODE threads)
endif()

set(COMMON_ARGS "${SCENE}" -q)
if(AA)
    list(APPEND COMMON_ARGS --aa ${AA})
endif()
if(COLOR)
    list(APPEND COMMON_ARGS --color ${COLOR})
endif()
if(SIZE)
    list(APPEND COMMON_ARGS --size ${SIZE})
endif()
if(TILE)
    list(APPEND COMMON_ARGS --tile ${TILE})
endif()

function(run_raythrower output)
    execute_process(COMMAND "${RAYTHROWER}" ${COMMON_ARGS} -o "${output}" ${ARGN}
                    RESULT_VARIABLE RESULT
                    OUTPUT_VARIABLE STDOUT
                    ERROR_VARIABLE  STDERR
                    TIMEOUT 900)
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "raythrower failed (${RESULT}):\n${STDOUT}\n${STDERR}")
    endif()
endfunction()

if(SELF_CHECK)
    string(REGEX REPLACE "\\.raw$" "" BASE "${OUTPUT}")
    set(EXACT_WITH "${BASE}_threads.raw")
    run_raythrower("${EXACT_WITH}" -N4)
endif()

if(MODE STREQUAL "network")
    if(NOT PORT)
        set(PORT 0)     # let the system pick a free one
    endif()
    run_raythrower("${OUTPUT}" -N3 --spawn --listen ${PORT} --timeout 60)
else()
    run_raythrower("${OUTPUT}" -N4)
endif()

if(REFERENCE)
    execute_process(COMMAND "${COMPARE}" "${OUTPUT}" "${REFERENCE}" RESULT_VARIABLE RESULT)
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "The render does not match ${REFERENCE}")
    endif()
endif()

if(EXACT_WITH)
    execute_process(COMMAND "${COMPARE}" "${OUTPUT}" "${EXACT_WITH}" --exact RESULT_VARIABLE RESULT)
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "The render is not identical to ${EXACT_WITH}")
    endif()
endif()
