# Fails if a source file calls min or max in a way that breaks on Windows.
#
#   cmake -DROOT=<source dir> -P LintMinMax.cmake
#
# Windows headers define min and max as macros, and the MFC build sees
# them, so std::min( a, b ), std::max( ... ) and
# std::numeric_limits<T>::max() fail to compile there. Write
# (std::min)( a, b ), (std::max)( a, b ) and
# (std::numeric_limits<T>::max)() instead. This runs on every platform, so
# the mistake shows up before the Windows build does.

if(NOT ROOT)
    message(FATAL_ERROR "ROOT is not set")
endif()

set(FOUND "")
foreach(DIR core src include editor)
    file(GLOB_RECURSE FILES "${ROOT}/${DIR}/*.cpp" "${ROOT}/${DIR}/*.h")
    foreach(FILE ${FILES})
        file(STRINGS "${FILE}" LINES)
        set(NUMBER 0)
        foreach(LINE IN LISTS LINES)
            math(EXPR NUMBER "${NUMBER} + 1")
            string(REGEX REPLACE "//.*$" "" CODE "${LINE}")
            if(CODE MATCHES "(^|[^(])std::(min|max)[ \t]*\\("
               OR CODE MATCHES "numeric_limits<[^>]*>::(min|max)[ \t]*\\(")
                file(RELATIVE_PATH NAME "${ROOT}" "${FILE}")
                string(STRIP "${LINE}" SHOWN)
                string(APPEND FOUND "  ${NAME}:${NUMBER}: ${SHOWN}\n")
            endif()
        endforeach()
    endforeach()
endforeach()

if(FOUND)
    message(FATAL_ERROR "min or max called without parentheses (breaks the Windows build):\n${FOUND}"
                        "Write (std::min)( a, b ), (std::max)( a, b ) or (std::numeric_limits<T>::max)().")
endif()
message(STATUS "No unparenthesized min or max calls.")
