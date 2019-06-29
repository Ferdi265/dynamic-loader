#include "dynlink.h"
#include "loader.h"
#include "deps.h"
#include "reloc.h"
#include "init.h"
#include "ld_malloc.h"
#include "debug.h"

dso_t * dso_resolve_dynload(char * name, bool exec, libpath_context_t * context) {
    dso_t * dso = dso_find(name);
    if (dso == NULL) {
        char * path = libpath_context_resolve(name, context);
        if (path == NULL) {
            ERROR(dynlink, "Failed to resolve '%s'\n", name);
            return NULL;
        }

        return dso_dynload(path, exec, context);
    } else {
        dso_ref(dso);
        return dso;
    }
}

dso_t * dso_dynload(char * path, bool exec, libpath_context_t * context) {
    dso_t * dso = dso_load(path);
    if (dso == NULL) {
        ERROR(dynlink, "Failed to load '%s'\n", path);
        ld_free(path);
        return NULL;
    }

    if (!dso_dynlink(dso, exec, context)) {
        return NULL;
    }

    return dso;
}

bool dso_dynlink(dso_t * dso, bool exec, libpath_context_t * context) {
    if (!dso_load_deps(dso, context)) {
        ERROR(dynlink, "Failed to load dependencies of '%s'\n", dso->path);
        dso_unref(dso);
        return false;
    }

    if (!dso_relocate(dso)) {
        ERROR(dynlink, "Failed to relocate '%s'\n", dso->path);
        dso_unref(dso);
        return false;
    }

    if (!exec) {
        dso_initialize(dso);
    }

    return true;
}

void dso_dynunload(dso_t * dso) {
    dso_unload_deps(dso);
    dso_unload(dso);
}
