file(GLOB common_sources *.c)
add_library(common STATIC ${common_sources})
target_include_directories(common PUBLIC .)
