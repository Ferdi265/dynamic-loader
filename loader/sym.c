#include "sym.h"
#include "debug.h"

void * resolve_symbol(char * name) {
    DEBUG(sym, "Trying to find symbol '%s'\n", name);
    dso_t * cur = loaded_objects;
    while (cur != NULL) {
        void * sym = dso_resolve_symbol(cur, name);
        if (sym != NULL) return sym;

        cur = cur->next;
    }

    return NULL;
}

void * dso_resolve_symbol(dso_t * dso, char * name) {
    DEBUG(sym, "Trying to find symbol in '%s'\n", dso->path);

    void * sym = gnu_hash_resolve_symbol(dso, name);
    if (sym != NULL) return sym;

    sym = elf_hash_resolve_symbol(dso, name);
    return sym;
}
