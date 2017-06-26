cmake_minimum_required (VERSION 2.8)
project(toy)

set(CMAKE_VERBOSE_MAKEFILE OFF)

add_compile_options(
    "-std=c++14"
    "-Wall"
)

include_directories(
"include"
"."
)

aux_source_directory("src/" SRC)
aux_source_directory("src/log" SRC)
aux_source_directory("src/feed" SRC)
aux_source_directory("src/reference" SRC)
add_executable(toy ${SRC})

target_link_libraries(toy
    pthread
)

