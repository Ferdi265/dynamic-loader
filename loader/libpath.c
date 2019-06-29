#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "debug.h"
#include "libpath.h"
#include "ld_malloc.h"

static char * base_paths[] = {
    "/home/yrlf/Programming/c/loader-test/shared"
};

libpath_context_t base_search_path = {
    .rpath = NULL,
    .envpath = {
        .paths = NULL,
        .length = 0
    },
    .runpath = NULL,
    .defpath = {
        .paths = base_paths,
        .length = (sizeof base_paths) / (sizeof (char *))
    }
};

libpath_list_t base_rpath;
libpath_list_t base_runpath;

bool parse_libpath(char * path, libpath_t * libpath) {
    DEBUG(libpath, "Parsing libpath '%s'\n", path);
    libpath->paths = NULL;
    libpath->length = 0;

    char * cur = path;
    while (*cur != '\0') {
        char * next = strchr(cur, ':');
        if (next == NULL) {
            next = cur + strlen(cur);
        }

        size_t length = next - cur;
        char * new_path = (char *)ld_malloc(length + 1);
        if (new_path == NULL) {
            ERROR(libpath, "Failed to allocate buffer for path name\n");

            free_libpath(libpath);
            return false;
        }
        strncpy(new_path, cur, length);
        new_path[length] = '\0';

        char ** new_paths = (char **)ld_realloc(libpath->paths, (libpath->length + 1) * sizeof (char *));
        if (new_paths == NULL) {
            ERROR(libpath, "Failed to resize library path pointer array\n");

            ld_free(new_path);
            free_libpath(libpath);
            return false;
        }
        libpath->paths = new_paths;
        libpath->paths[libpath->length++] = new_path;

        DEBUG(libpath, "- '%s'\n", new_path);

        cur = next + (*next != '\0');
    }

    return true;
}

void free_libpath(libpath_t * libpath) {
    for (size_t i = 0; i < libpath->length; i++) {
        ld_free(libpath->paths[i]);
    }
    ld_free(libpath->paths);
    libpath->paths = NULL;
    libpath->length = 0;
}

void load_libpath() {
    char * env_libpath = getenv("LD_LIBRARY_PATH");
    if (env_libpath != NULL) {
        DEBUG(libpath, "Parsing LD_LIBRARY_PATH\n");
        parse_libpath(env_libpath, &base_search_path.envpath);
    }

    char * env_rpath = getenv("LD_RPATH");
    if (env_rpath != NULL) {
        DEBUG(libpath, "Parsing LD_RPATH\n");
        base_search_path.rpath = &base_rpath;
        parse_libpath(env_rpath, &base_rpath.element);
    }

    char * env_runpath = getenv("LD_RUNPATH");
    if (env_runpath != NULL) {
        DEBUG(libpath, "Parsing LD_RUNPATH\n");
        base_search_path.runpath = &base_runpath;
        parse_libpath(env_runpath, &base_runpath.element);
    }
}

void unload_libpath() {
    free_libpath(&base_search_path.envpath);
    free_libpath(&base_rpath.element);
    free_libpath(&base_runpath.element);
}

char * libpath_context_resolve(char * name, libpath_context_t * context) {
    char * path;
    DEBUG(libpath, "Trying to resolve '%s'\n", name);

    DEBUG(libpath, "Trying rpath...\n");
    path = libpath_list_resolve(name, context->rpath);
    if (path != NULL) return path;
    DEBUG(libpath, "- no match\n");

    DEBUG(libpath, "Trying envpath...\n");
    path = libpath_resolve(name, &context->envpath);
    if (path != NULL) return path;
    DEBUG(libpath, "- no match\n");

    DEBUG(libpath, "Trying runpath...\n");
    path = libpath_list_resolve(name, context->runpath);
    if (path != NULL) return path;
    DEBUG(libpath, "- no match\n");

    DEBUG(libpath, "Trying defpath...\n");
    path = libpath_resolve(name, &context->defpath);
    if (path != NULL) return path;
    DEBUG(libpath, "- no match\n");

    DEBUG(libpath, "Failed to resolve '%s'\n", name);
    return NULL;
}

char * libpath_list_resolve(char * name, libpath_list_t * libpaths) {
    libpath_list_t * cur = libpaths;
    while (cur != NULL) {
        char * path = libpath_resolve(name, &cur->element);
        if (path != NULL) return path;

        cur = cur->next;
    }

    return NULL;
}

char * libpath_resolve(char * name, libpath_t * libpath) {
    if (*name == '/') {
        char * new_path = strdup(name);
        if (new_path == NULL) {
            ERROR(libpath, "Failed to allocate path buffer\n");
        }
        return new_path;
    }

    char * path = NULL;
    size_t path_len = 0;
    size_t name_len = strlen(name);
    for (size_t i = 0; i < libpath->length; i++) {
        size_t new_path_len = name_len + 1 + strlen(libpath->paths[i]) + 1;
        if (new_path_len > path_len) {
            char * new_path = (char *)ld_realloc(path, new_path_len);
            if (new_path == NULL) {
                ld_free(path);
                ERROR(libpath, "Failed to grow path buffer\n");
                return NULL;
            } else {
                path = new_path;
            }
        }

        strcpy(path, libpath->paths[i]);
        strcat(path, "/");
        strcat(path, name);

        if (access(path, R_OK) == 0) {
            char * new_path = (char *)ld_realloc(path, strlen(path) + 1);
            if (new_path == NULL) {
                ERROR(libpath, "Failed to shrink path buffer\n");
                ld_free(path);
                return NULL;
            }
            return new_path;
        }
    }

    ld_free(path);
    return NULL;
}
