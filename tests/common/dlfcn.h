#pragma once

void * dlopen(char * name, int flags);
int dlclose(void * handle);

void * dlsym(void * handle, char * symbol);
