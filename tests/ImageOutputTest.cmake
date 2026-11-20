# Renders a scene with --png and --hdr and checks both files against the raw
# image with tools/check_image.py.
#
#   cmake -DRAYTHROWER=... -DPYTHON=... -DCHECK=... -DSCENE=... -DOUTPUT=<base>
#         -P ImageOutputTest.cmake

execute_process(COMMAND "${RAYTHROWER}" "${SCENE}" -q -N4 -o "${OUTPUT}.raw" --png "${OUTPUT}.png" --hdr "${OUTPUT}.hdr"
                RESULT_VARIABLE RESULT OUTPUT_VARIABLE STDOUT ERROR_VARIABLE STDERR TIMEOUT 300)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "raythrower failed (${RESULT}):\n${STDOUT}\n${STDERR}")
endif()

foreach(check "--png;${OUTPUT}.png" "--hdr;${OUTPUT}.hdr;--clip")
    execute_process(COMMAND "${PYTHON}" -I "${CHECK}" "${OUTPUT}.raw" ${check}
                    RESULT_VARIABLE RESULT OUTPUT_VARIABLE STDOUT ERROR_VARIABLE STDERR)
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "${check}: ${STDOUT}${STDERR}")
    endif()
endforeach()
