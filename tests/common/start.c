#include "syscall.h"
#include "crt.h"

void _init() {}
void _fini() {}

__attribute__((noreturn))
int __libc_start_main(mainfn * main, int argc, char ** argv, voidfn * init, voidfn * fini, void (*rtld_fini)(void), void * stack_end) {
    char ** envp = &argv[argc + 1];
    int ret = main(argc, argv, envp);

    __syscall(SYS_exit, ret, 0, 0, 0, 0, 0);
    __builtin_unreachable();
}
