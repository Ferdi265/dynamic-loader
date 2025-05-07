#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef void voidfn();
typedef int mainfn(int, char **, char **);

int main(int argc, char ** argv, char ** envp);
[[noreturn]]
int __libc_start_main(mainfn * main, int argc, char ** argv, voidfn * init, voidfn * fini, void (*rtld_fini)(void), void * stack_end);
void _init();
void _fini();
void _start_c(size_t * sp);

extern bool __is_loader;
extern void * __loader_base;
