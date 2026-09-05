cmake_minimum_required(VERSION 3.16)

project(Balin VERSION 0.1.0 LANGUAGES C)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

add_executable(balin
    src/main.c
    src/colors.c
    src/sdl.c
)

target_link_libraries(balin PRIVATE SDL3::SDL3)

target_compile_options(balin PRIVATE -Wall -Wextra)

set_target_properties(balin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)