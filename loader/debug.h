#pragma once

#include <stdio.h>
#include <stdbool.h>

#define ERROR(flag, ...) \
    if (debug_ ## flag || true) fprintf(stderr, "[LD_ERROR][" #flag "] " __VA_ARGS__)

#define DEBUG(flag, ...) \
    if (debug_ ## flag) fprintf(stderr, "[LD_DEBUG][" #flag "] " __VA_ARGS__)

extern bool debug_debug;
extern bool debug_env;
extern bool debug_main;
extern bool debug_elf;
extern bool debug_load;
extern bool debug_deps;
extern bool debug_sym;
extern bool debug_reloc;
extern bool debug_init;
extern bool debug_dynlink;
extern bool debug_libpath;
extern bool debug_libsummary;

void load_debug_flags();
