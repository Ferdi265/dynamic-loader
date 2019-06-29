#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "debug.h"

bool debug_debug = false;
bool debug_env = false;
bool debug_main = false;
bool debug_elf = false;
bool debug_load = false;
bool debug_deps = false;
bool debug_sym = false;
bool debug_reloc = false;
bool debug_init = false;
bool debug_dynlink = false;
bool debug_libpath = false;
bool debug_libsummary = false;

#define ENABLE_DEBUG(flag) do {\
        if (!debug_ ## flag) { \
            debug_ ## flag = true; \
            DEBUG(debug, "Enabling debug flag '" #flag "'\n"); \
        } \
    } while (0)

void load_debug_flags() {
    char * env_debug = getenv("LD_DEBUG");
    if (env_debug == NULL) return;

    char * cur = env_debug;
    while (*cur != '\0') {
        char * next = strchr(cur, ':');
        if (next == NULL) {
            next = cur + strlen(cur);
        }

        #define CHECK_FLAG_ALL \
            if (strncmp("all", cur, length) == 0) { \
                ENABLE_DEBUG(debug); \
                ENABLE_DEBUG(env); \
                ENABLE_DEBUG(main); \
                ENABLE_DEBUG(elf); \
                ENABLE_DEBUG(load); \
                ENABLE_DEBUG(deps); \
                ENABLE_DEBUG(sym); \
                ENABLE_DEBUG(reloc); \
                ENABLE_DEBUG(init); \
                ENABLE_DEBUG(dynlink); \
                ENABLE_DEBUG(libpath); \
                ENABLE_DEBUG(libsummary); \
            }
        #define CHECK_FLAG(flag) \
            if (strncmp(#flag, cur, length) == 0) { \
                ENABLE_DEBUG(flag); \
            }

        size_t length = next - cur;

        CHECK_FLAG_ALL
        else CHECK_FLAG(debug)
        else CHECK_FLAG(env)
        else CHECK_FLAG(main)
        else CHECK_FLAG(elf)
        else CHECK_FLAG(load)
        else CHECK_FLAG(deps)
        else CHECK_FLAG(sym)
        else CHECK_FLAG(reloc)
        else CHECK_FLAG(init)
        else CHECK_FLAG(dynlink)
        else CHECK_FLAG(libpath)
        else CHECK_FLAG(libsummary)
        else {
            char buf[length + 1];
            strncpy(buf, cur, length);
            buf[length] = '\0';

            ERROR(debug, "Unknown debug flag '%s'\n", buf);
        }

        cur = next + (*next != '\0');
    }
}
