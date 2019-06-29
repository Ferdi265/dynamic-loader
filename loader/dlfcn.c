#include "dynlink.h"
#include "libpath.h"
#include "sym.h"
#include "dlfcn.h"

void * dlopen(char * name, int flags) {
    (void)flags;

    pthread_mutex_lock(&dso_lock);
    dso_t * dso = dso_resolve_dynload(name, false, &base_search_path);
    pthread_mutex_unlock(&dso_lock);

    return (void *)dso;
}

int dlclose(void * handle) {
    dso_t * dso = (dso_t *)handle;

    pthread_mutex_lock(&dso_lock);
    dso_unref(dso);
    pthread_mutex_unlock(&dso_lock);

    return 0;
}

void * dlsym(void * handle, char * symbol) {
    dso_t * dso = (dso_t *)handle;
    void * ret;

    pthread_mutex_lock(&dso_lock);
    if (dso == NULL) {
        ret = resolve_symbol(symbol);
    } else {
        ret = dso_resolve_symbol(dso, symbol);
    }
    pthread_mutex_unlock(&dso_lock);

    return ret;
}
