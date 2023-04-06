# dynamic-loader

A simple dynamic library loader for Linux capable of loading simple binaries using simple shared libraries.

## Features

- Actually readable codebase for a dynamic loader
- Loads simple shared objects (no glibc or musl support\*)
- Calls initializers and finalizers
- Performs relocations at load-time (no lazy loading supported yet)
- Supports `dlopen()`, `dlsym()`, and `dlclose()`
- no support for threads or TLS, though rudimentary thread-safety is supported
  in the loader

\* glibc and musl provide their own dynamic library loader that they expect to
be loaded with:
- `ld-linux-x86_64.so.2` (aka `ld.so`) for glibc
- `ld-musl-x86_64.so.1` (a symlink to musl's `libc.so`) for musl

## Dependencies

- `libmusl.a` and `musl-gcc` (arch: `musl`, debian: `musl musl-tools`)
- `elf.h` (arch: part of `base`, debian: `libelf-dev`)

## Building

- `cmake -B build`
- `cd build`
- `make`
- run `build/loader/libloader.so` or any of the binaries in `build/tests/bin/`
