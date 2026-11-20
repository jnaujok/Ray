# Renders a scene with raythrower and compares the result.
#
#   cmake -DRAYTHROWER=... -DCOMPARE=... -DSCENE=... -DOUTPUT=...
#         [-DREFERENCE=...] [-DMODE=threads|network] [-DEXACT_WITH=...]
#         [-DAA=<sampler>] [-DCOLOR=<color model>] [-DSIZE=<w>x<h>]
#         -P RenderTest.cmake
#
# MODE network renders with rayengine processes over TCP. EXACT_WITH names
# another render the output must match byte for byte. AA and COLOR pick the
# sampler and color model instead of the scene's.

if(NOT MODE)
    set(MODE threads)
endif()

set(ARGS "${SCENE}" -q -o "${OUTPUT}")
if(AA)
    list(APPEND ARGS --aa ${AA})
endif()
if(COLOR)
    list(APPEND ARGS --color ${COLOR})
endif()
if(SIZE)
    list(APPEND ARGS --size ${SIZE})
endif()
if(MODE STREQUAL "network")
    if(NOT PORT)
        set(PORT 0)     # let the system pick a free one
    endif()
    list(APPEND ARGS -N3 --spawn --listen ${PORT} --timeout 60)
else()
    list(APPEND ARGS -N4)
endif()

execute_process(COMMAND "${RAYTHROWER}" ${ARGS}
                RESULT_VARIABLE RESULT
                OUTPUT_VARIABLE STDOUT
                ERROR_VARIABLE  STDERR
                TIMEOUT 900)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "raythrower failed (${RESULT}):\n${STDOUT}\n${STDERR}")
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
