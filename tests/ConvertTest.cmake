# Converts a 1997 binary scene to JSON and checks the result is exactly the
# JSON scene file in the repository.
#
#   cmake -DRAYTHROWER=... -DLEGACY=... -DEXPECTED=... -DOUTPUT=... -P ConvertTest.cmake

execute_process(COMMAND "${RAYTHROWER}" "${LEGACY}" -q --convert "${OUTPUT}"
                RESULT_VARIABLE RESULT
                OUTPUT_VARIABLE STDOUT
                ERROR_VARIABLE  STDERR)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "raythrower --convert failed (${RESULT}):\n${STDOUT}\n${STDERR}")
endif()

file(READ "${OUTPUT}" CONVERTED)
file(READ "${EXPECTED}" COMMITTED)
if(NOT CONVERTED STREQUAL COMMITTED)
    message(FATAL_ERROR "Converting ${LEGACY} does not give ${EXPECTED}")
endif()
