add_library(crt_object STATIC crt.c)
set(CRT_OBJECT_PATH ${CMAKE_CURRENT_BINARY_DIR}/libcrt_object.a PARENT_SCOPE)

add_library(static_pie_crt_object STATIC static-pie-crt.c)
target_compile_options(static_pie_crt_object PRIVATE -fpie)
set(STATIC_PIE_CRT_OBJECT_PATH ${CMAKE_CURRENT_BINARY_DIR}/libstatic_pie_crt_object.a PARENT_SCOPE)

execute_process(COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
add_library(crt_objects STATIC ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
add_dependencies(crt_objects crt_object static_pie_crt_object)
target_include_directories(crt_objects INTERFACE .)
