#pragma once

#include <stdbool.h>

#include "dso.h"
#include "libpath.h"

bool dso_load_deps(dso_t * dso, libpath_context_t * context);
void dso_unload_deps(dso_t * dso);
