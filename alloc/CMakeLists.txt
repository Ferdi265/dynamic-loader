add_library(alloc INTERFACE)

if(${WITH_CUSTOM_ALLOCATOR})
    file(GLOB_RECURSE alloc_sources *.c)
    target_sources(alloc INTERFACE ${alloc_sources})
endif()
