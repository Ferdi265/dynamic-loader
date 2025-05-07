#include <string.h>

[[gnu::constructor]]
void init() {
    puts("I'm an initializer");
}

[[gnu::constructor]]
void init2() {
    puts("I'm a second initializer");
}

void foo() {
    puts("I'm a library function");
}
