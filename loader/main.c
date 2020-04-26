#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <elf.h>

#include "crt.h"
#include "dso.h"
#include "loader.h"
#include "dynlink.h"
#include "debug.h"
#include "libpath.h"
#include "env.h"

int main(int argc, char ** argv, char ** envp) {
    size_t * sp = (size_t *)(argv - 1);
    size_t * auxvals = (size_t *)envp;
    while (*auxvals++);

    load_env();

    pthread_mutex_lock(&dso_lock);
    if (dso_load_self() == NULL) {
        ERROR(main, "Failed to create dso handle for the loader\n");
        exit(1);
    }

    char * name = NULL;
    if (__is_loader) {
        name = argv[0];
    } else {
        if (argc < 2) {
            ERROR(main, "Invalid number of parameters\n");
            ERROR(main, "Usage: %s <elf_file> [args...]\n", argc > 0 ? argv[0] : "loader");
            exit(1);
        }

        name = argv[1];
    }

    for (size_t i = 0; i < preload_list.length; i++) {
        dso_dynload(preload_list.paths[i], false, &base_search_path);
    }

    dso_t * dso = NULL;
    if (__is_loader) {
        Elf64_Phdr * phdr = NULL;
        size_t phdr_length = 0;
        void * entry = NULL;
        for (size_t i = 0; auxvals[i] != AT_NULL; i += 2) {
            size_t val = auxvals[i + 1];
            switch (auxvals[i]) {
                case AT_PHDR:
                    phdr = (Elf64_Phdr *)val;
                    break;
                case AT_PHNUM:
                    phdr_length = val;
                    break;
                case AT_ENTRY:
                    entry = (void *)val;
                    break;
            }
        }

        dso = dso_load_initial(name, phdr, phdr_length, entry);
        if (!dso_dynlink(dso, true, &base_search_path)) {
            dso = NULL;
        }
    } else {
        dso = dso_dynload(name, true, &base_search_path);

        *(size_t *)argv = argc - 1;
        argv++;
        sp++;
    }

    if (dso == NULL) {
        ERROR(main, "Could not load '%s'\n", name);
        exit(1);
    }

    DEBUG(libsummary, "Loaded Libraries:\n");
    dso_t * cur = loaded_objects;
    while (cur != NULL) {
        DEBUG(libsummary, "- %s (at %p, refcount = %zd)\n", cur->path, cur->base, cur->refcount);
        cur = cur->next;
    }

    if (env_noexec) {
        DEBUG(main, "LD_NOEXEC is set, not executing\n");

        dso_unref(dso);
        dso_unload_self();
        unload_libpath();

        DEBUG(libsummary, "Libraries still loaded on exit:\n");
        cur = loaded_objects;
        while (cur != NULL) {
            DEBUG(libsummary, "- %s (at %p, refcount = %zd)\n", cur->path, cur->base, cur->refcount);
            cur = cur->next;
        }

        pthread_mutex_unlock(&dso_lock);
        exit(0);
    } else {
        sp[-1] = (size_t)dso->entry;
        pthread_mutex_unlock(&dso_lock);

        asm(
            "mov %[sp], %%rsp\n\t"
            "mov $0, %%rax\n\t"
            "mov $0, %%rbx\n\t"
            "mov $0, %%rcx\n\t"
            "mov $0, %%rdx\n\t"
            "mov $0, %%rdi\n\t"
            "mov $0, %%rsi\n\t"
            "mov $0, %%rbp\n\t"
            "mov $0, %%r8\n\t"
            "mov $0, %%r9\n\t"
            "mov $0, %%r10\n\t"
            "mov $0, %%r11\n\t"
            "mov $0, %%r12\n\t"
            "mov $0, %%r13\n\t"
            "mov $0, %%r14\n\t"
            "mov $0, %%r15\n\t"
            "ret\n\t"
            :: [sp]"r"(sp - 1)
        );
    }
}
