link_libraries(common)

add_library(string SHARED string.c)

add_library(init SHARED init.c)
target_link_libraries(init PRIVATE string)

add_library(test SHARED test.c)
target_link_libraries(test PRIVATE string)
