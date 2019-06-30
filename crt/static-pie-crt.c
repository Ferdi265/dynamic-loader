#include <elf.h>

#include "crt.h"

bool __is_loader;
void * __loader_base;

__attribute__((visibility("hidden")))
extern Elf64_Dyn _DYNAMIC[];

__attribute__((naked, noreturn))
void _start() {
    asm(
        "endbr64\n\t"
        "xor %%rbp, %%rbp\n\t"
        "mov %%rsp, %%rdi\n\t"
        "andq $-16, %%rsp\n\t"
        "jmp _start_c\n\t"
        :::
    );
}

__attribute__((noreturn))
void _start_c(size_t * sp) {
    int argc = *sp;
    char ** argv = (char **)(sp + 1);
    char ** envp = &argv[argc + 1];
    size_t * auxvals = (size_t *)envp;
    while (*auxvals++);

    char * base = NULL;
    Elf64_Phdr * phdrs = NULL;
    size_t phdrs_length = 0;
    for (size_t i = 0; auxvals[i] != AT_NULL; i += 2) {
        size_t val = auxvals[i + 1];
        switch (auxvals[i]) {
            case AT_BASE:
                base = (char *)val;
                break;
            case AT_PHDR:
                phdrs = (Elf64_Phdr *)val;
                break;
            case AT_PHNUM:
                phdrs_length = val;
                break;
        }
    }

    Elf64_Dyn * dynamic = _DYNAMIC;
    bool dyn_loader = base != NULL;
    if (!dyn_loader) {
        for (size_t i = 0; i < phdrs_length; i++) {
            if (phdrs[i].p_type == PT_DYNAMIC) {
                base = (char *)(((size_t)dynamic) - phdrs[i].p_vaddr);
                break;
            }
        }
    }

    Elf64_Rela * rela = NULL;
    size_t rela_length = 0;
    for (size_t i = 0; dynamic[i].d_tag != DT_NULL; i++) {
        switch (dynamic[i].d_tag) {
            case DT_RELA:
                rela = (Elf64_Rela *)(base + dynamic[i].d_un.d_ptr);
                break;
            case DT_RELASZ:
                rela_length = dynamic[i].d_un.d_val / sizeof (Elf64_Rela);
                break;
        }
    }

    for (size_t i = 0; i < rela_length; i++) {
        if (ELF64_R_TYPE(rela[i].r_info) != R_X86_64_RELATIVE) continue;

        uint64_t * target = (uint64_t *)(base + rela[i].r_offset);
        *target = (size_t)base + rela[i].r_addend;
    }

    __is_loader = dyn_loader;
    __loader_base = base;
    __libc_start_main(main, argc, argv, _init, _fini, NULL, NULL);
}
