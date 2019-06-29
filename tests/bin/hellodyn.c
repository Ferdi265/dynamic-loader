#include "dlfcn.h"
#include "string.h"

typedef void voidfn_t();

int main() {
    puts("Hello!");

    void * handle = dlopen("libtest.so", 0);
    if (handle == NULL) {
        puts("Failed :(");
    } else {
        puts("libtest.so loaded, calling test...");

        voidfn_t * test = dlsym(handle, "test");
        if (test == NULL) {
            puts("No test?");
        } else {
            test();
        }

        puts("closing handle...");
        dlclose(handle);
    }

    puts("getting puts function pointer...");
    voidfn_t * puts_fn = dlsym(NULL, "puts");
    if (puts_fn != NULL) {
        puts_fn("Dynamic puts :O");
    }

    puts("Bye!");
}
