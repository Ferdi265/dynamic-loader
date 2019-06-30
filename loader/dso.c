#include "dso.h"
#include "util.h"
#include "init.h"
#include "dynlink.h"

pthread_mutex_t dso_lock = PTHREAD_MUTEX_INITIALIZER;
dso_t * loaded_objects = NULL;
dso_t * last_loaded = NULL;

static bool dso_linked(dso_t * dso) {
    dso_t * cur = loaded_objects;
    while (cur != NULL) {
        if (cur == dso) {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

static void dso_link(dso_t * dso) {
    if (last_loaded == NULL) {
        loaded_objects = dso;
        last_loaded = dso;
    } else {
        last_loaded->next = dso;
        dso->last = last_loaded;
        last_loaded = dso;
    }
}

static void dso_unlink(dso_t * dso) {
    if (dso->initialized) {
        dso_finalize(dso);
    }

    if (dso->refcount > 1) return;

    if (dso->last != NULL) {
        dso->last->next = dso->next;
    }
    if (dso->next != NULL) {
        dso->next->last = dso->last;
    }
    if (loaded_objects == dso) {
        loaded_objects = dso->next;
    }
    if (last_loaded == dso) {
        last_loaded = dso->last;
    }

    dso_dynunload(dso);
}

void dso_ref(dso_t * dso) {
    dso->refcount++;

    if (!dso_linked(dso)) dso_link(dso);
}

void dso_unref(dso_t * dso) {
    if (dso->refcount == 1) {
        dso_unlink(dso);
    } else {
        dso->refcount--;
    }
}

dso_t * dso_find(char * name) {
    dso_t * cur = loaded_objects;
    while (cur != NULL) {
        if (pathmatch(name, cur->path)) return cur;
        cur = cur->next;
    }
    return NULL;
}
