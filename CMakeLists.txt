cmake_minimum_required(VERSION 3.12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_SKIP_BUILD_RPATH ON)

add_compile_options(-specs loader-gcc.specs)
add_link_options(-specs loader-gcc.specs)

add_subdirectory(crt)
add_subdirectory(loader)
add_subdirectory(tests)

configure_file(loader-gcc.specs.in loader-gcc.specs @ONLY)
