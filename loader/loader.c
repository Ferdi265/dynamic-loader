#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>

#include "crt.h"
#include "elf.h"
#include "dso.h"
#include "debug.h"
#include "ld_malloc.h"

#define PAGE_SIZE 0x1000
#define FLOOR2(v, n) ((v) & ~((n) - 1))
#define CEIL2(v, n) (((v) + (n) - 1) & ~((n) - 1))

typedef struct {
    size_t size;
    int fd;
    Elf64_Ehdr * header;
} elf_file_t;

static bool open_elf(char * path, elf_file_t * info) {
    info->fd = open(path, O_RDONLY);
    if (info->fd == -1) {
        ERROR(load, "Failed to open library '%s'\n", path);
        return false;
    }

    size_t size = lseek(info->fd, 0, SEEK_END);
    if (size == (size_t)-1) {
        close(info->fd);
        ERROR(load, "Failed to get file size\n");
        return false;
    }

    Elf64_Ehdr * header = (Elf64_Ehdr *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, info->fd, 0);
    if (header == MAP_FAILED) {
        close(info->fd);
        ERROR(load, "Failed to map file\n");
        return false;
    }

    info->size = size;
    info->header = header;
    return true;
}

static void close_elf(elf_file_t * info) {
    close(info->fd);
    munmap(info->header, info->size);
}

static bool unmap_load_segments(char * base, Elf64_Phdr * phdrs, size_t phdr_length) {
    for (uint16_t i = 0; i < phdr_length; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            Elf64_Phdr * cur = &phdrs[i];

            size_t vaddr_page = FLOOR2(cur->p_vaddr, PAGE_SIZE);
            size_t align_diff = cur->p_vaddr - vaddr_page;
            size_t memsz_page = CEIL2(align_diff + cur->p_memsz, PAGE_SIZE);
            if (munmap(base + vaddr_page, memsz_page) != 0) {
                ERROR(load, "Failed to unmap program header %d\n", i);
                return false;
            }
        }
    }

    return true;
}

static bool map_load_segments(elf_file_t * info, char ** out_base) {
    char * base = NULL;
    if (info->header->e_type == ET_DYN) {
        getrandom(&base, sizeof (char *), 0);
        base = (char *)((((size_t)base) & FLOOR2(0x00000fffffffffff, PAGE_SIZE)) | 0x0000600000000000);
    }

    Elf64_Phdr * phdrs = (Elf64_Phdr *)(((char *)info->header) + info->header->e_phoff);
    uint16_t i;
    for (i = 0; i < info->header->e_phnum; i++) {
        Elf64_Phdr * cur = &phdrs[i];
        if (cur->p_type == PT_LOAD) {
            int prot =
                ((cur->p_flags & PF_R) ? PROT_READ : 0) |
                ((cur->p_flags & PF_W) ? PROT_WRITE : 0) |
                ((cur->p_flags & PF_X) ? PROT_EXEC : 0);

            size_t vaddr_page = FLOOR2(cur->p_vaddr, PAGE_SIZE);
            size_t offset_page = FLOOR2(cur->p_offset, PAGE_SIZE);
            size_t align_diff = cur->p_vaddr - vaddr_page;

            size_t filesz_page = CEIL2(align_diff + cur->p_filesz, PAGE_SIZE);
            size_t memsz_page = CEIL2(align_diff + cur->p_memsz, PAGE_SIZE);

            int mmap_prot = (align_diff > 0 ? PROT_READ | PROT_WRITE : prot);

            char * map = (char *)mmap(base + vaddr_page, filesz_page, mmap_prot, MAP_FIXED | MAP_PRIVATE, info->fd, offset_page);
            if (map == MAP_FAILED) {
                ERROR(load, "Failed to map program header %d (mapped from file)\n", i);
                goto fail;
            }

            if (align_diff > 0) {
                memset(map, 0, align_diff);
                if (prot != mmap_prot && mprotect(map, align_diff + cur->p_memsz, prot) == -1) {
                    ERROR(load, "Failed to reprotect program header %d (mapped from file)\n", i);
                    goto fail;
                }
            }

            if (memsz_page > filesz_page) {
                map = (char *)mmap(base + vaddr_page + filesz_page, memsz_page - filesz_page, prot, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (map == MAP_FAILED) {
                    ERROR(load, "Failed to map program header %d (mapped anonymously)\n", i);
                    goto fail;
                }
            }
        }
    }

    *out_base = base;
    return true;

fail:
    if (!unmap_load_segments(base, phdrs, info->header->e_phnum)) exit(1);
    return false;
}

static Elf64_Phdr * copy_phdrs(Elf64_Phdr * phdr, size_t phdr_length) {
    Elf64_Phdr * copy = (Elf64_Phdr *)ld_malloc(phdr_length * sizeof (Elf64_Phdr));
    if (copy == NULL) {
        ERROR(load, "Failed to allocate space for program header copy\n");
        return NULL;
    }

    memcpy(copy, phdr, phdr_length * sizeof (Elf64_Phdr));
    return copy;
}

dso_t * dso_load(char * path) {
    elf_file_t info;
    if (!open_elf(path, &info)) return NULL;

    Elf64_Phdr * phdr = (Elf64_Phdr *)((char *)info.header + info.header->e_phoff);
    size_t phdr_length = info.header->e_phnum;
    if (!elf_verify_header(info.header)) goto close_fail;

    char * base;
    if (!map_load_segments(&info, &base)) goto close_fail;

    Elf64_Phdr * copy = copy_phdrs(phdr, phdr_length);
    if (copy == NULL) goto close_fail;

    if (!elf_verify_dynamic(phdr, phdr_length, base)) {
        ERROR(load, "ELF is missing information needed for dynamic loading\n");
        close_elf(&info);
        unmap_load_segments(base, copy, phdr_length);
        ld_free(copy);
        return NULL;
    }

    dso_t * dso = (dso_t *)ld_malloc(sizeof (dso_t));
    if (dso == NULL) {
        ERROR(load, "Failed to allocate dso handle\n");
        ld_free(copy);
        goto close_fail;
    }

    dso->path = path;
    dso->base = base;
    dso->phdr = copy;
    dso->phdr_length = phdr_length;
    dso->dyn = phdr_get_dyn(copy, phdr_length, base);
    dso->entry = (void *)(base + info.header->e_entry);
    dso->unloadable = true;
    dso->initialized = false;
    dso->refcount = 0;
    dso->deps = (dso_deps_t){ .elements = NULL, .length = 0 };
    dso->last = NULL;
    dso->next = NULL;

    close_elf(&info);
    dso_ref(dso);
    return dso;

close_fail:
    close_elf(&info);
    return NULL;
}

dso_t * dso_load_initial(char * name, Elf64_Phdr * phdr, size_t phdr_length, void * entry) {
    char * path = strdup(name);
    if (path == NULL) return NULL;

    dso_t * dso = (dso_t *)ld_malloc(sizeof (dso_t));
    if (dso == NULL) {
        ERROR(load, "Failed to allocate dso handle\n");
        goto path_fail;
    }

    Elf64_Phdr * phdr_ent = phdr_get_ent(phdr, phdr_length, PT_PHDR);
    if (phdr_ent == NULL) {
        ERROR(load, "Failed to get binary base (no PT_PHDR entry present)\n");
        goto path_fail;
    }
    dso->base = (char *)phdr - phdr_ent->p_vaddr;

    if (!elf_verify_dynamic(phdr, phdr_length, dso->base)) {
        ERROR(load, "ELF is missing information needed for dynamic loading\n");
        goto path_fail;
    }

    Elf64_Phdr * copy = copy_phdrs(phdr, phdr_length);
    if (copy == NULL) {
        ERROR(load, "Failed to allocate phdr copy\n");
        goto path_fail;
    }

    dso->path = path;
    dso->phdr = copy;
    dso->phdr_length = phdr_length;
    dso->dyn = phdr_get_dyn(copy, phdr_length, dso->base);
    dso->entry = entry;
    dso->unloadable = false;
    dso->initialized = false;
    dso->refcount = 0;
    dso->deps = (dso_deps_t){ .elements = NULL, .length = 0 };
    dso->next = NULL;
    dso->last = NULL;

    dso_ref(dso);
    return dso;

path_fail:
    ld_free(path);
    return NULL;
}

extern Elf64_Dyn _DYNAMIC[];
static dso_t * self = NULL;
dso_t * dso_load_self() {
    if (self != NULL) return self;

    char * path = strdup("loader.so");
    if (path == NULL) return NULL;

    dso_t * dso = (dso_t *)ld_malloc(sizeof (dso_t));
    if (dso == NULL) {
        ERROR(load, "Failed to allocate dso handle\n");
        goto path_fail;
    }

    dso->base = __loader_base;
    dso->path = path;
    dso->phdr = NULL;
    dso->phdr_length = 0;
    dso->dyn = _DYNAMIC;
    dso->entry = NULL;
    dso->unloadable = false;
    dso->initialized = true;
    dso->refcount = 0;
    dso->deps = (dso_deps_t){ .elements = NULL, .length = 0 };
    dso->next = NULL;
    dso->last = NULL;

    dso_ref(dso);
    return self = dso;

path_fail:
    ld_free(path);
    return NULL;
}

bool dso_unload(dso_t * dso) {
    bool res = true;
    if (dso->unloadable) {
        res = unmap_load_segments(dso->base, dso->phdr, dso->phdr_length);
    }

    ld_free(dso->deps.elements);
    ld_free(dso->phdr);
    ld_free(dso->path);
    ld_free(dso);
    return res;
}

void dso_unload_self() {
    if (self != NULL) dso_unref(self);
    self = NULL;
}
