# The rays a render casts, by kind, must be the same every time: with any
# number of threads, and with networked engines.
#
#   cmake -DRAYTHROWER=... -DSCENE=... -DOUTPUT=<directory> -DSIZE=<w>x<h>
#         [-DPORT=<port>] -P RayCountTest.cmake
#
# Renders the scene with 4 threads, 2 threads and 3 networked engines, each
# writing --stats-json, and compares the "rays" counts. Each must also have
# cast some camera rays and some rays of another kind.

if(NOT PORT)
    set(PORT 0)
endif()

set(RUNS threads4 threads2 network)
foreach(run IN LISTS RUNS)
    set(JSON "${OUTPUT}/raycount_${run}.json")
    set(ARGS "${SCENE}" -q -o "${OUTPUT}/raycount_${run}.raw" --size ${SIZE} --tile 16 --stats-json "${JSON}")
    if(run STREQUAL "threads4")
        list(APPEND ARGS -N4)
    elseif(run STREQUAL "threads2")
        list(APPEND ARGS -N2)
    else()
        list(APPEND ARGS -N3 --spawn --listen ${PORT} --timeout 60)
    endif()
    execute_process(COMMAND "${RAYTHROWER}" ${ARGS} RESULT_VARIABLE RESULT OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR TIMEOUT 600)
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "raythrower (${run}) failed: ${RESULT}\n${OUT}\n${ERR}")
    endif()
    file(READ "${JSON}" TEXT)
    string(JSON RAYS GET "${TEXT}" rays)
    set(RAYS_${run} "${RAYS}")
endforeach()

string(JSON CAMERA GET "${RAYS_threads4}" camera)
string(JSON TOTAL GET "${RAYS_threads4}" total)
if(CAMERA LESS_EQUAL 0 OR TOTAL LESS_EQUAL CAMERA)
    message(FATAL_ERROR "the render cast ${CAMERA} camera rays of ${TOTAL}: expected camera rays and others")
endif()
foreach(run IN ITEMS threads2 network)
    string(JSON SAME EQUAL "${RAYS_threads4}" "${RAYS_${run}}")
    if(NOT SAME)
        message(FATAL_ERROR "the ray counts differ between 4 threads and ${run}:\n${RAYS_threads4}\n${RAYS_${run}}")
    endif()
endforeach()
message(STATUS "ray counts match: ${RAYS_threads4}")
