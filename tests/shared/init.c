#include <string.h>

__attribute__((constructor))
void init() {
    puts("I'm an initializer");
}

__attribute__((constructor))
void init2() {
    puts("I'm a second initializer");
}

void foo() {
    puts("I'm a library function");
}
