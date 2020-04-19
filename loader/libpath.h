#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char ** paths;
    size_t length;
} libpath_t;

typedef struct _libpath_list {
    libpath_t element;
    struct _libpath_list * next;
} libpath_list_t;

typedef struct {
    libpath_list_t * rpath;
    libpath_t envpath;
    libpath_list_t * runpath;
    libpath_t defpath;
} libpath_context_t;

extern libpath_context_t base_search_path;
extern libpath_t preload_list;

bool parse_libpath(char * path, libpath_t * libpath);
void free_libpath(libpath_t * libpath);
void load_libpath();
void unload_libpath();

char * libpath_context_resolve(char * name, libpath_context_t * context);
char * libpath_list_resolve(char * name, libpath_list_t * libpaths);
char * libpath_resolve(char * name, libpath_t * libpath);
