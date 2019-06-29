#pragma once

#include "dso.h"

typedef struct {
    uint32_t num_buckets;
    uint32_t num_chains;
} elf_hash_table_t;

typedef struct {
    uint32_t num_buckets;
    uint32_t sym_offset;
    uint32_t bloom_size;
    uint32_t bloom_shift;
} gnu_hash_table_t;

#define ELF_HASH_BUCKETS(tab) \
    ((uint32_t *)(((char *)(tab)) + sizeof (elf_hash_table_t)))

#define ELF_HASH_CHAINS(tab) \
    (&ELF_HASH_BUCKETS(tab)[tab->num_buckets])

#define GNU_HASH_BLOOM(tab) \
    ((uint64_t *)(((char *)(tab)) + sizeof (gnu_hash_table_t)))

#define GNU_HASH_BUCKETS(tab) \
    ((uint32_t *)(&GNU_HASH_BLOOM(tab)[tab->bloom_size]))

#define GNU_HASH_CHAINS(tab) \
    (&GNU_HASH_BUCKETS(tab)[tab->num_buckets])

uint32_t elf_hash(char * name);
uint32_t gnu_hash(char * name);

void * elf_hash_resolve_symbol(dso_t * dso, char * name);
void * gnu_hash_resolve_symbol(dso_t * dso, char * name);

void * resolve_symbol(char * name);
void * dso_resolve_symbol(dso_t * dso, char * name);
