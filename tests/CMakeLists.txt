add_library(dynamic_library INTERFACE)
target_link_options(dynamic_library INTERFACE -nodefaultlibs)
target_link_libraries(dynamic_library INTERFACE dynamic_loader)

add_library(dynamic_executable INTERFACE)
target_compile_options(dynamic_executable INTERFACE -pie)
target_link_options(dynamic_executable INTERFACE -pie -nodefaultlibs)
target_link_libraries(dynamic_executable INTERFACE dynamic_loader pie_crt_object)

add_subdirectory(common)
add_subdirectory(shared)
add_subdirectory(bin)
