#include "string.h"

__attribute__((constructor))
void init() {
    puts("[libtest.so] Loaded");
}

__attribute__((destructor))
void fini() {
    puts("[libtest.so] Unloaded");
}

void test() {
    puts("It's working!");
}
