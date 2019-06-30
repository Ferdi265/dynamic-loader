link_libraries(common)

add_executable(hello hello.c)

add_executable(helloshared helloshared.c)
target_link_libraries(helloshared PRIVATE string)

add_executable(helloargs helloargs.c)
target_link_libraries(helloargs PRIVATE string)

add_executable(helloinit helloinit.c)
target_link_libraries(helloinit PRIVATE string init)

add_executable(hellodyn hellodyn.c)
target_link_libraries(hellodyn PRIVATE string loader)
