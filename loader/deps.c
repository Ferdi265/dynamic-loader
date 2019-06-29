#include "deps.h"
#include "dynlink.h"
#include "elf.h"
#include "ld_malloc.h"
#include "debug.h"

bool dso_load_deps(dso_t * dso, libpath_context_t * context) {
    dso_ref(dso);
    char * strtab = dyn_get_strtab(dso->dyn, dso->base);

    libpath_context_t local_context = *context;

    libpath_list_t rpath = { .element = { .paths = NULL, .length = 0 }, .next = NULL };
    char * rpath_str = dyn_get_rpath(dso->dyn, strtab);
    if (rpath_str != NULL) {
        DEBUG(deps, "Parsing RPATH of '%s'\n", dso->path);
        if (parse_libpath(rpath_str, &rpath.element)) {
            rpath.next = local_context.rpath;
            local_context.rpath = &rpath;
        }
    }

    libpath_list_t runpath = { .element = { .paths = NULL, .length = 0 }, .next = NULL };
    char * runpath_str = dyn_get_runpath(dso->dyn, strtab);
    if (runpath_str != NULL) {
        DEBUG(deps, "Parsing RUNPATH of '%s'\n", dso->path);
        if (parse_libpath(runpath_str, &runpath.element)) {
            runpath.next = local_context.runpath;
            local_context.runpath = &runpath;
        }
    }

    for (size_t i = 0; dso->dyn[i].d_tag != DT_NULL; i++) {
        if (dso->dyn[i].d_tag == DT_NEEDED) {
            dso_t ** new_deps_list = ld_realloc(dso->deps.elements, (dso->deps.length + 1) * sizeof (dso_t *));
            if (new_deps_list == NULL) {
                ERROR(deps, "Failed to grow dependency list\n");

                free_libpath(&rpath.element);
                free_libpath(&runpath.element);
                dso_unload_deps(dso);
                dso_unref(dso);
                return false;
            }
            dso->deps.elements = new_deps_list;

            char * name = &strtab[dso->dyn[i].d_un.d_val];
            dso_t * dep = dso_resolve_dynload(name, false, &local_context);
            if (dep == NULL) {
                ERROR(deps, "Failed to load dependency '%s'\n", name);

                free_libpath(&rpath.element);
                free_libpath(&runpath.element);
                dso_unload_deps(dso);
                dso_unref(dso);
                return false;
            }
            dso->deps.elements[dso->deps.length++] = dep;
        }
    }

    free_libpath(&rpath.element);
    free_libpath(&runpath.element);
    dso_unref(dso);
    return true;
}

void dso_unload_deps(dso_t * dso) {
    for (size_t i = 0; i < dso->deps.length; i++) {
        dso_unref(dso->deps.elements[i]);
    }
    ld_free(dso->deps.elements);
    dso->deps.elements = NULL;
    dso->deps.length = 0;
}

