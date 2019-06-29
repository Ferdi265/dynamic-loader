#include <stdlib.h>

#include "env.h"
#include "debug.h"
#include "libpath.h"

bool env_noexec = false;

#define LOAD_BOOL_ENV(var, flag) do {\
        char * str = getenv(var); \
        if (str != NULL) { \
            char * end = NULL; \
            unsigned long val = strtoul(str, &end, 10); \
            if (*end != '\0') { ERROR(env, "Unable to parse " var ": '%s' is not a number\n", str); } \
            else { env_ ## flag = val; } \
        } \
    } while (0)

void load_env() {
    load_debug_flags();
    load_libpath();

    LOAD_BOOL_ENV("LD_NOEXEC", noexec);
}
