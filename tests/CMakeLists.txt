link_libraries(crt_objects dynamic_loader)
add_link_options(-pie)

add_subdirectory(common)
add_subdirectory(shared)
add_subdirectory(bin)
