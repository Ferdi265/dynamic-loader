#include <unistd.h>
#include "syscall.h"

int main() {
    __syscall(SYS_write, STDOUT_FILENO, (size_t)"HELLO\n", 6, 0, 0, 0);
}
