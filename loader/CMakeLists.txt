file(GLOB_RECURSE loader_sources *.c)

set(BASE_LIBRARY_PATH ${CMAKE_BINARY_DIR}/tests/shared)
configure_file(basepath.h.in basepath.h @ONLY ESCAPE_QUOTES)

add_library(loader SHARED ${loader_sources})
target_link_options(loader PRIVATE -Wl,-soname=libloader.so -Wl,-z,now -Wl,-Bstatic -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/dynamic-list.txt)
target_link_libraries(loader PRIVATE static_pie_crt_object crt_headers)
target_link_libraries(loader INTERFACE $<TARGET_FILE:loader>)
target_include_directories(loader PUBLIC include PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set(LOADER_OBJECT_PATH ${CMAKE_CURRENT_BINARY_DIR}/libloader.so PARENT_SCOPE)

add_library(dynamic_loader INTERFACE)
target_link_options(dynamic_loader INTERFACE -Wl,-dynamic-linker,$<TARGET_FILE:loader>)
add_dependencies(dynamic_loader loader)
