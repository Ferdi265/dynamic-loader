link_libraries(crt_objects)

file(GLOB_RECURSE loader_sources *.c)

set(BASE_LIBRARY_PATH ${CMAKE_BINARY_DIR}/tests/shared)
configure_file(basepath.h.in basepath.h @ONLY ESCAPE_QUOTES)

add_executable(loader ${loader_sources})
set_target_properties(loader PROPERTIES PREFIX lib SUFFIX .so)
set_target_properties(loader PROPERTIES ENABLE_EXPORTS ON)
target_link_options(loader PRIVATE -static-pie -Wl,-soname=libloader.so -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/dynamic-list.txt)
target_link_libraries(loader INTERFACE $<TARGET_FILE:loader>)
target_include_directories(loader PUBLIC include PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set(LOADER_OBJECT_PATH ${CMAKE_CURRENT_BINARY_DIR}/libloader.so PARENT_SCOPE)

execute_process(COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
add_library(dynamic_loader STATIC ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
add_dependencies(dynamic_loader loader)
