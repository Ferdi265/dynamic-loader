#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <elf.h>

#include "sym.h"

typedef void voidfn_t();

bool elf_verify_header(Elf64_Ehdr * header);
bool elf_verify_dynamic(Elf64_Phdr * phdr, size_t phdrs, char * base);
bool dyn_verify_dynamic(Elf64_Dyn * phdr, char * base);

Elf64_Phdr * phdr_get_ent(Elf64_Phdr * phdr, size_t phdrs, uint32_t type);
Elf64_Dyn * phdr_get_dyn(Elf64_Phdr * phdr, size_t phdrs, char * base);

Elf64_Dyn * dyn_get_ent(Elf64_Dyn * dyn, int64_t tag);
Elf64_Sym * dyn_get_symtab(Elf64_Dyn * dyn, char * base);
elf_hash_table_t * dyn_get_elf_hash(Elf64_Dyn * dyn, char * base);
gnu_hash_table_t * dyn_get_gnu_hash(Elf64_Dyn * dyn, char * base);
char * dyn_get_strtab(Elf64_Dyn * dyn, char * base);
char * dyn_get_soname(Elf64_Dyn * dyn, char * strtab);
Elf64_Rela * dyn_get_rela(Elf64_Dyn * dyn, char * base);
size_t dyn_get_rela_length(Elf64_Dyn * dyn);
Elf64_Rela * dyn_get_pltrel(Elf64_Dyn * dyn, char * base);
size_t dyn_get_pltrel_length(Elf64_Dyn * dyn);
char * dyn_get_pltgot(Elf64_Dyn * dyn, char * base);
char * dyn_get_rpath(Elf64_Dyn * dyn, char * strtab);
char * dyn_get_runpath(Elf64_Dyn * dyn, char * strtab);
voidfn_t * dyn_get_init(Elf64_Dyn * dyn, char * base);
voidfn_t ** dyn_get_init_array(Elf64_Dyn * dyn, char * base);
size_t dyn_get_init_array_length(Elf64_Dyn * dyn);
voidfn_t * dyn_get_fini(Elf64_Dyn * dyn, char * base);
voidfn_t ** dyn_get_fini_array(Elf64_Dyn * dyn, char * base);
size_t dyn_get_fini_array_length(Elf64_Dyn * dyn);
