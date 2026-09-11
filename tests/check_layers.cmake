# ADR-002 dependency rule, run by ctest as the layer_rule test.
#   engine must not include game or tools headers
#   game must not include tools headers
#
# Usage: cmake -DSRC_DIR=<repo>/src -P tests/check_layers.cmake

if(NOT SRC_DIR)
    message(FATAL_ERROR "SRC_DIR is required")
endif()

foreach(layer IN ITEMS engine game tools)
    if(NOT IS_DIRECTORY "${SRC_DIR}/${layer}")
        message(FATAL_ERROR "Missing module folder: ${SRC_DIR}/${layer}")
    endif()
endforeach()

set(violations "")

set(checks
    "engine:game"
    "engine:tools"
    "game:tools"
)

foreach(check IN LISTS checks)
    string(REPLACE ":" ";" pair "${check}")
    list(GET pair 0 layer)
    list(GET pair 1 forbidden)

    file(GLOB_RECURSE files
        "${SRC_DIR}/${layer}/*.c"
        "${SRC_DIR}/${layer}/*.h"
    )

    foreach(file IN LISTS files)
        file(READ "${file}" content)
        if(content MATCHES "#[ \t]*include[ \t]*[<\"](\\.\\./)*(src/)?${forbidden}/")
            list(APPEND violations "${file} includes ${forbidden}/")
        endif()
    endforeach()
endforeach()

if(violations)
    string(REPLACE ";" "\n  " text "${violations}")
    message(FATAL_ERROR "ADR-002 layer rule violated:\n  ${text}")
endif()

message(STATUS "Layer rule check passed")
