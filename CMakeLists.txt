cmake_minimum_required(VERSION 3.16)

project(Balin VERSION 0.1.0 LANGUAGES C)

# Debug is the default build type. Request Release explicitly.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# BALIN_DEV compiles the tools module. On in Debug by default, off otherwise.
if(DEFINED BALIN_DEV)
    set(BALIN_DEV_DEFAULT ${BALIN_DEV})
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BALIN_DEV_DEFAULT ON)
else()
    set(BALIN_DEV_DEFAULT OFF)
endif()
option(BALIN_DEV "Build dev tools (editor, debug overlay)" ${BALIN_DEV_DEFAULT})

# Compile commands for clangd and editor tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendor/raylib)

# Per ADR-016: Debug gets ASan and UBSan, Release is -O2. Vendored raylib stays untouched.
function(balin_flags target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O2>
    )

    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-fsanitize=address,undefined>
            $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        )
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        )
    endif()
endfunction()

set(BALIN_SOURCES
    src/engine/arena.c
    src/engine/log.c
    src/main.c
    src/game/game.c
    src/game/memory.c
)

if(BALIN_DEV)
    list(APPEND BALIN_SOURCES
        # Dev tool sources land here: src/tools/...
    )
endif()

add_executable(balin ${BALIN_SOURCES})
target_include_directories(balin PRIVATE src)
target_link_libraries(balin PRIVATE raylib)
target_compile_definitions(balin PRIVATE BALIN_DEV=$<BOOL:${BALIN_DEV}>)
balin_flags(balin)

add_executable(balin_tests
    tests/test_main.c
    src/engine/arena.c
    src/engine/log.c
    src/game/memory.c
)
target_include_directories(balin_tests PRIVATE src)
target_compile_definitions(balin_tests PRIVATE BALIN_DEV=$<BOOL:${BALIN_DEV}>)
balin_flags(balin_tests)

# CMake defines NDEBUG in Release. Keep assert active in the test binary.
target_compile_options(balin_tests PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)

set_target_properties(balin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)

enable_testing()

add_test(NAME balin_tests COMMAND balin_tests)
add_test(NAME layer_rule COMMAND ${CMAKE_COMMAND}
    -DSRC_DIR=${CMAKE_SOURCE_DIR}/src
    -P ${CMAKE_SOURCE_DIR}/tests/check_layers.cmake
)
