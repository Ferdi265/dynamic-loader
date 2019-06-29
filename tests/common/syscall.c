#include "syscall.h"

size_t __syscall(size_t nr, size_t a0, size_t a1, size_t a2, size_t a3, size_t a4, size_t a5) {
    size_t ret;
    asm volatile(
        "mov %[a3], %%r10\n\t"
        "mov %[a4], %%r8\n\t"
        "mov %[a5], %%r9\n\t"
        "syscall\n\t"
        : "=a"(ret)
        : "a"(nr), "D"(a0), "S"(a1), "d"(a2), [a3]"rm"(a3), [a4]"rm"(a4), [a5]"rm"(a5)
        : "rcx", "r8", "r9", "r10"
    );
    return ret;
}

