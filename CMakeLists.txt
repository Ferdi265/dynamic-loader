cmake_minimum_required(VERSION 3.12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_SKIP_BUILD_RPATH ON)

set(CMAKE_C_COMPILER musl-gcc)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)

project(dynamic_loader C)

option(WITH_CUSTOM_ALLOCATOR "use the custom mmap-only malloc implementation" ON)

add_subdirectory(crt)
add_subdirectory(alloc)
add_subdirectory(loader)
add_subdirectory(tests)
