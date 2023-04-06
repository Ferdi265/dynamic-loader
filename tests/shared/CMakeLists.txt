add_library(string SHARED string.c)
target_link_libraries(string PRIVATE dynamic_library common)

add_library(init SHARED init.c)
target_link_libraries(init PRIVATE dynamic_library common string)

add_library(test SHARED test.c)
target_link_libraries(test PRIVATE dynamic_library common string)
