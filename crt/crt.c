#include "crt.h"

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

    __libc_start_main(main, argc, argv, _init, _fini, NULL, NULL);
}
