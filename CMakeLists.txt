cmake_minimum_required(VERSION 3.16)

project(Balin VERSION 0.1.0 LANGUAGES C)

add_subdirectory(vendor/raylib)


add_executable(balin
    src/main.c
    src/world.c
    src/entity.c
    src/input.c
)

target_link_libraries(balin PRIVATE raylib)

target_compile_options(balin PRIVATE -Wall -Wextra)

set_target_properties(balin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)