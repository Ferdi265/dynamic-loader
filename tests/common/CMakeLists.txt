file(GLOB common_sources *.c)
add_library(common STATIC ${common_sources})
target_link_libraries(common PRIVATE crt_headers)
target_include_directories(common PUBLIC .)
