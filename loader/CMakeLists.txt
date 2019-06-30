link_libraries(crt_objects)

file(GLOB_RECURSE loader_sources *.c)

add_executable(loader ${loader_sources})
set_target_properties(loader PROPERTIES PREFIX lib SUFFIX .so)
set_target_properties(loader PROPERTIES ENABLE_EXPORTS ON)
target_link_options(loader PRIVATE -static-pie -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/dynamic-list.txt)
target_link_libraries(loader INTERFACE $<TARGET_FILE:loader>)
target_include_directories(loader PUBLIC include)
set(LOADER_OBJECT_PATH ${CMAKE_CURRENT_BINARY_DIR}/libloader.so PARENT_SCOPE)

execute_process(COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
add_library(dynamic_loader STATIC ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
add_dependencies(dynamic_loader loader)
