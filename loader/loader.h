#pragma once

#include <elf.h>

#include "dso.h"

dso_t * dso_load(char * path);
dso_t * dso_load_initial(char * name, Elf64_Phdr * phdr, size_t phdr_length, void * entry);
dso_t * dso_load_self();
void dso_unload(dso_t * dso);
void dso_unload_self();
