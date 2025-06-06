#include "syscall.h"
#include "crt.h"

[[noreturn]]
int __libc_start_main(mainfn * main, int argc, char ** argv, voidfn * init, voidfn * fini, [[maybe_unused]] void (*rtld_fini)(void), [[maybe_unused]] void * stack_end) {
    char ** envp = &argv[argc + 1];

    init();
    int ret = main(argc, argv, envp);
    fini();

    __syscall(SYS_exit, ret, 0, 0, 0, 0, 0);
    __builtin_unreachable();
}
