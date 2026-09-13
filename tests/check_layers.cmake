# ADR-002 dependency rule, run by ctest as the layer_rule test.
#   engine must not include game or tools headers
#   game must not include tools headers
#
# ADR-022 leaf rule: a leaf folder may include headers from its own layer
# only from its own folder. A leaf may still include from another layer,
# which ADR-002 already governs. Add a new leaf here when it lands.
#   engine/core may include only engine/core headers
#   game/ecs may include only game/ecs headers
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

# Layer rules: a file in one layer must not include from a forbidden layer.
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

# Leaf rules: same-layer quoted includes must start with the leaf path.
set(leaf_checks
    "engine/core:engine"
    "game/ecs:game"
)

foreach(check IN LISTS leaf_checks)
    string(REPLACE ":" ";" pair "${check}")
    list(GET pair 0 leaf)
    list(GET pair 1 layer)

    if(NOT IS_DIRECTORY "${SRC_DIR}/${leaf}")
        message(FATAL_ERROR "Missing leaf folder: ${SRC_DIR}/${leaf}")
    endif()

    file(GLOB_RECURSE files
        "${SRC_DIR}/${leaf}/*.c"
        "${SRC_DIR}/${leaf}/*.h"
    )

    foreach(file IN LISTS files)
        file(READ "${file}" content)
        string(REGEX MATCHALL "#[ \t]*include[ \t]*\"[^\"]*\"" includes "${content}")
        foreach(include IN LISTS includes)
            if(include MATCHES "\"(${layer}/[^\"]+)\"")
                set(path "${CMAKE_MATCH_1}")
                string(FIND "${path}" "${leaf}/" position)
                if(NOT position EQUAL 0)
                    list(APPEND violations "${file} includes ${path} (leaf ${leaf})")
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()

if(violations)
    string(REPLACE ";" "\n  " text "${violations}")
    message(FATAL_ERROR "ADR-002/ADR-022 layer rule violated:\n  ${text}")
endif()

message(STATUS "Layer rule check passed")
