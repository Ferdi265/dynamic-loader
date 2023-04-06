add_executable(hello hello.c)
target_link_libraries(hello PRIVATE dynamic_executable common)

add_executable(helloshared helloshared.c)
target_link_libraries(helloshared PRIVATE dynamic_executable common string)

add_executable(helloargs helloargs.c)
target_link_libraries(helloargs PRIVATE dynamic_executable common string)

add_executable(helloinit helloinit.c)
target_link_libraries(helloinit PRIVATE dynamic_executable common string init)

add_executable(hellodyn hellodyn.c)
target_link_libraries(hellodyn PRIVATE dynamic_executable common string loader)
