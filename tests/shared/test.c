#include "string.h"

[[gnu::constructor]]
void init() {
    puts("[libtest.so] Loaded");
}

[[gnu::destructor]]
void fini() {
    puts("[libtest.so] Unloaded");
}

void test() {
    puts("It's working!");
}
