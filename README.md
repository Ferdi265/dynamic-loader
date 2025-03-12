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

## Resources

This loader was written mostly by using specifications from
[refspecs.linuxbase.org](https://refspecs.linuxbase.org/), mainly the
[ELF spec](https://refspecs.linuxfoundation.org/elf/elf.pdf) and the
[x86\_64 processor supplement](https://refspecs.linuxfoundation.org/elf/x86_64-abi-0.95.pdf).

For some parts of the early loader startup code (mainly the code in `crt/`), I
also read the source code of [musl libc](musl-libc.org/) to get a better
understanding of typical ways to handle early loader init. To understand ELF
symbol hash tables, I used multiple sources, but
[this blog post on flagpenguin.me](https://flapenguin.me/elf-dt-hash) was most
useful.

## License

This project is licensed under the MIT License (SPDX
[MIT](https://spdx.org/licenses/MIT.html)). The full
license text can also be found in the [LICENSE](/LICENSE) file.
