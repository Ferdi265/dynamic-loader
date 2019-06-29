#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <elf.h>

struct _dso;
typedef struct {
    struct _dso ** elements;
    size_t length;
} dso_deps_t;

typedef struct _dso {
    char * path;
    char * base;
    Elf64_Phdr * phdr;
    size_t phdr_length;
    Elf64_Dyn * dyn;
    void * entry;
    bool unloadable;
    bool initialized;
    size_t refcount;
    dso_deps_t deps;
    struct _dso * next;
    struct _dso * last;
} dso_t;


extern pthread_mutex_t dso_lock;
extern dso_t * loaded_objects;
extern dso_t * last_loaded;

void dso_ref(dso_t * dso);
void dso_unref(dso_t * dso);
dso_t * dso_find(char * name);
