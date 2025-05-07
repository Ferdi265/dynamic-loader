#include "init.h"
#include "env.h"
#include "elf.h"
#include "debug.h"

void dso_initialize(dso_t * dso) {
    DEBUG(init, "Initializing '%s'\n", dso->path);
    dso_ref(dso);
    voidfn_t * init = dyn_get_init(dso->dyn, dso->base);
    if (init != NULL && !env_noexec) {
        DEBUG(init, "Calling initializer INIT at %p\n", (void *)init);

        pthread_mutex_unlock(&dso_lock);
        init();
        pthread_mutex_lock(&dso_lock);
    }

    voidfn_t ** init_array = dyn_get_init_array(dso->dyn, dso->base);
    size_t init_array_length =  dyn_get_init_array_length(dso->dyn);
    if (init_array != NULL) {
        DEBUG(init, "Processing INIT_ARRAY of length %zd\n", init_array_length);
        for (size_t i = 0; i < init_array_length; i++) {
            if (!env_noexec) {
                DEBUG(init, "Calling initializer INIT_ARRAY[%zd] at %p\n", i, (void *)init_array[i]);

                pthread_mutex_unlock(&dso_lock);
                init_array[i]();
                pthread_mutex_lock(&dso_lock);
            }
        }
    }

    DEBUG(init, "Finished initializing '%s'\n", dso->path);
    dso_unref(dso);
    dso->initialized = true;
}

void dso_finalize(dso_t * dso) {
    DEBUG(init, "Finalizing '%s'\n", dso->path);
    dso_ref(dso);
    voidfn_t * fini = dyn_get_fini(dso->dyn, dso->base);
    if (fini != NULL && !env_noexec) {
        DEBUG(init, "Calling finalizer FINI at %p\n", (void *)fini);

        pthread_mutex_unlock(&dso_lock);
        fini();
        pthread_mutex_lock(&dso_lock);
    }

    voidfn_t ** fini_array = dyn_get_fini_array(dso->dyn, dso->base);
    size_t fini_array_length =  dyn_get_fini_array_length(dso->dyn);
    if (fini_array != NULL) {
        DEBUG(init, "Processing FINI_ARRAY of length %zd\n", fini_array_length);
        for (size_t i = 0; i < fini_array_length; i++) {
            if (!env_noexec) {
                DEBUG(init, "Calling finalizer FINI_ARRAY[%zd] at %p\n", i, (void *)fini_array[i]);

                pthread_mutex_unlock(&dso_lock);
                fini_array[i]();
                pthread_mutex_lock(&dso_lock);
            }
        }
    }

    DEBUG(init, "Finished finalizing '%s'\n", dso->path);
    dso_unref(dso);
}
