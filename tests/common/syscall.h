#pragma once

#include <stddef.h>
#include <sys/syscall.h>

__attribute__((weak, visibility("hidden")))
size_t __syscall(size_t nr, size_t a0, size_t a1, size_t a2, size_t a3, size_t a4, size_t a5);
