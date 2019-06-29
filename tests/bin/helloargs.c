#include "string.h"

int main(int argc, char ** argv) {
    puts("Program Arguments:");
    for (int i = 0; i < argc; i++) {
        puts(argv[i]);
    }
}
