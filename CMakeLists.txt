cmake_minimum_required(VERSION 3.10)

project(editor)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_executable(editor ${SRC_FILES})

target_compile_options(editor PUBLIC -Wall)

target_link_libraries(editor ncurses)
