#pragma once

#include <stddef.h>

void * ld_malloc(size_t size);
void ld_free(void * ptr);
void * ld_realloc(void * ptr, size_t size);
void * ld_calloc(size_t nmemb, size_t size);
