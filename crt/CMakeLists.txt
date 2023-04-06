add_library(pie_crt_object STATIC pie-crt.c)

add_library(static_pie_crt_object STATIC static-pie-crt.c)
target_compile_options(static_pie_crt_object PRIVATE -fpie)

add_library(crt_headers INTERFACE)
target_include_directories(crt_headers INTERFACE .)
