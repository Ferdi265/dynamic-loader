#include "util.h"

#include <string.h>

char * basename(char * str) {
    char * base = str;
    while (*str) {
        if (*str == '/') base = str + 1;
        str++;
    }
    return base;
}

bool pathmatch(char * name, char * path) {
    if (*name == '/') {
        return strcmp(name, path) == 0;
    } else {
        return strcmp(name, basename(path)) == 0;
    }
}
