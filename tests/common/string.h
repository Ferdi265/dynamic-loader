#pragma once

#include <stddef.h>
#include <sys/types.h>

ssize_t write(int fd, const void * buf, size_t count);

size_t strlen(const char * s);

int puts(const char * s);
