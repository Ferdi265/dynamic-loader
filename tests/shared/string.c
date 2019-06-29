#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "syscall.h"
#include "string.h"

ssize_t write(int fd, const void * buf, size_t count) {
    return (ssize_t)__syscall(SYS_write, (size_t)fd, (size_t)buf, count, 0, 0, 0);
}

size_t strlen(const char * s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

int puts(const char * s) {
    ssize_t ret1 = write(STDOUT_FILENO, s, strlen(s));
    if (ret1 < 0) return EOF;

    ssize_t ret2 = write(STDOUT_FILENO, "\n", 1);
    if (ret2 < 0) return EOF;

    return ret1 + ret2;
}
