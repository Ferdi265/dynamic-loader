#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "sym.h"
#include "elf.h"
#include "debug.h"

uint32_t elf_hash(char * name) {
    uint32_t hash = 0;
    uint32_t top_nibble;

    for (; *name; name++) {
        hash = (hash << 4) + *name;
        top_nibble = hash & 0xf0000000;

        if (top_nibble) {
            hash ^= top_nibble >> 24;
        }

        hash &= ~top_nibble;
    }
    return hash;
}

uint32_t gnu_hash(char * name) {
    uint32_t hash = 5381;

    for (; *name; name++) {
        hash = (hash << 5) + hash + *name;
    }

    return hash;
}

void * elf_hash_resolve_symbol(dso_t * dso, char * name) {
    Elf64_Sym * symtab = dyn_get_symtab(dso->dyn, dso->base);
    char * strtab = dyn_get_strtab(dso->dyn, dso->base);

    elf_hash_table_t * htab = dyn_get_elf_hash(dso->dyn, dso->base);
    if (htab == NULL) return NULL;

    uint32_t hash = elf_hash(name);
    DEBUG(sym, "Looking for hash = %08x\n", hash);

    uint32_t sym_idx = ELF_HASH_BUCKETS(htab)[hash % htab->num_buckets];

    Elf64_Sym * found = NULL;
    while (sym_idx != STN_UNDEF) {
        Elf64_Sym * cur = &symtab[sym_idx];

        if (cur->st_name != 0) {
            char * sym_name = &strtab[cur->st_name];
            if (strcmp(name, sym_name) == 0) {
                found = cur;
                break;
            }
        }

        sym_idx = ELF_HASH_CHAINS(htab)[sym_idx];
    }

    if (found == NULL) return NULL;

    if (found->st_shndx == SHN_UNDEF) {
        DEBUG(sym, "Symbol is undefined, but referenced\n");
        return NULL;
    }

    DEBUG(sym, "Symbol is defined, returning\n");
    return dso->base + found->st_value;
}

void * gnu_hash_resolve_symbol(dso_t * dso, char * name) {
    Elf64_Sym * symtab = dyn_get_symtab(dso->dyn, dso->base);
    char * strtab = dyn_get_strtab(dso->dyn, dso->base);

    gnu_hash_table_t * htab = dyn_get_gnu_hash(dso->dyn, dso->base);
    if (htab == NULL) return NULL;

    uint32_t hash = gnu_hash(name);
    DEBUG(sym, "Looking for gnu hash = %08x\n", hash);

    const size_t elfclass_bits = 64;
    uint64_t bloom = GNU_HASH_BLOOM(htab)[(hash / elfclass_bits) % htab->bloom_size];
    uint64_t mask = 0
        | ((uint64_t)1 << (hash % elfclass_bits))
        | ((uint64_t)1 << ((hash >> htab->bloom_shift) % elfclass_bits));

    if ((bloom & mask) != mask) return NULL;

    uint32_t sym_idx = GNU_HASH_BUCKETS(htab)[hash % htab->num_buckets];
    if (sym_idx < htab->sym_offset) return NULL;

    Elf64_Sym * found = NULL;
    while (true) {
        uint32_t chain_hash = GNU_HASH_CHAINS(htab)[sym_idx - htab->sym_offset];
        Elf64_Sym * cur = &symtab[sym_idx];

        if ((hash | 1) == (chain_hash | 1) && cur->st_name != 0) {
            char * sym_name = &strtab[cur->st_name];
            if (strcmp(name, sym_name) == 0) {
                found = cur;
                break;
            }
        }

        if (chain_hash & 1) break;
        sym_idx++;
    }

    if (found == NULL) return NULL;

    if (found->st_shndx == SHN_UNDEF) {
        DEBUG(sym, "Symbol is undefined, but referenced\n");
        return NULL;
    }

    DEBUG(sym, "Symbol is defined, returning\n");
    return dso->base + found->st_value;
}
