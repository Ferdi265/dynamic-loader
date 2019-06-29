#pragma once

#include <stdbool.h>

#include "dso.h"
#include "libpath.h"

dso_t * dso_resolve_dynload(char * name, bool exec, libpath_context_t * context);
dso_t * dso_dynload(char * path, bool exec, libpath_context_t * context);
bool dso_dynlink(dso_t * dso, bool exec, libpath_context_t * context);
void dso_dynunload(dso_t * dso);
