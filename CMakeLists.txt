cmake_minimum_required(VERSION 3.12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SKIP_BUILD_RPATH ON)

set(CMAKE_C_COMPILER musl-gcc)

project(dynamic_loader C)

add_subdirectory(crt)
add_subdirectory(loader)
add_subdirectory(tests)
