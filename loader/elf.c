#include "elf.h"
#include "debug.h"

bool elf_verify_header(Elf64_Ehdr * header) {
    if (
        header->e_ident[EI_MAG0] != ELFMAG0 ||
        header->e_ident[EI_MAG1] != ELFMAG1 ||
        header->e_ident[EI_MAG2] != ELFMAG2 ||
        header->e_ident[EI_MAG3] != ELFMAG3
    ) {
        ERROR(elf, "ELF ident magic incorrect: '%02x%02x%02x%02x', should be '%02x%02x%02x%02x' (ELFMAG)\n",
            header->e_ident[EI_MAG0],
            header->e_ident[EI_MAG1],
            header->e_ident[EI_MAG2],
            header->e_ident[EI_MAG3],
            ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3
        );
        return false;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS64) {
        ERROR(elf, "ELF ident class incorrect: '%02x', should be '%02x' (ELFCLASS64)\n",
            header->e_ident[EI_CLASS],
            ELFCLASS64
        );
        return false;
    }

    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        ERROR(elf, "ELF ident data format incorrect: '%02x', should be '%02x' (ELFDATA2LSB)\n",
            header->e_ident[EI_DATA],
            ELFDATA2LSB
        );
        return false;
    }

    if (header->e_ident[EI_VERSION] != EV_CURRENT) {
        ERROR(elf, "ELF ident version incorrect: '%02x', should be '%02x' (EV_CURRENT)\n",
            header->e_ident[EI_VERSION],
            EV_CURRENT
        );
        return false;
    }

    if (
        header->e_ident[EI_OSABI] != ELFOSABI_SYSV &&
        header->e_ident[EI_OSABI] != ELFOSABI_LINUX
    ) {
        ERROR(elf, "ELF ident OS ABI incorrect: '%02x', should be '%02x' (ELFOSABI_SYSV) or '%02x' (ELFOSABI_LINUX)\n",
            header->e_ident[EI_OSABI],
            ELFOSABI_SYSV, ELFOSABI_LINUX
        );
        return false;
    }

    if (header->e_ident[EI_ABIVERSION] != 0) {
        ERROR(elf, "ELF ident ABI version incorrect: '%02x', should be '%02x'\n",
            header->e_ident[EI_ABIVERSION],
            0
        );
        return false;
    }

    if (
        header->e_type != ET_EXEC &&
        header->e_type != ET_DYN
    ) {
        ERROR(elf, "ELF type incorrect: '%04x', should be '%04x' (ET_EXEC) or '%04x' (ET_DYN)\n",
            header->e_type,
            ET_EXEC,
            ET_DYN
        );
        return false;
    }

    if (header->e_machine != EM_X86_64) {
        ERROR(elf, "ELF machine incorrect: '%04x', should be '%04x' (EM_X86_64)\n",
            header->e_machine,
            EM_X86_64
        );
        return false;
    }

    if (header->e_version != EV_CURRENT) {
        ERROR(elf, "ELF version incorrect: '%08x', should be '%08x' (EV_CURRENT)\n",
            header->e_version,
            EV_CURRENT
        );
        return false;
    }

    if (header->e_ehsize != sizeof (Elf64_Ehdr)) {
        ERROR(elf, "ELF header size: '%04x', should be '%04x'\n",
            header->e_ehsize,
            (uint16_t) sizeof (Elf64_Ehdr)
        );
        return false;
    }

    return true;
}

bool elf_verify_dynamic(Elf64_Phdr * phdr, size_t phdrs, char * base) {
    if (phdr == NULL) {
        ERROR(elf, "ELF has no program headers\n");
        return false;
    }

    Elf64_Dyn * dynamic = phdr_get_dyn(phdr, phdrs, base);
    if (dynamic == NULL) {
        ERROR(elf, "ELF has no dynamic segment\n");
        return false;
    }

    Elf64_Sym * symtab = dyn_get_symtab(dynamic, base);
    if (symtab == NULL) {
        ERROR(elf, "ELF has no symbol table\n");
        return false;
    }

    char * strtab = dyn_get_strtab(dynamic, base);
    if (strtab == NULL) {
        ERROR(elf, "ELF has no string table\n");
        return false;
    }

    elf_hash_table_t * elf_hash = dyn_get_elf_hash(dynamic, base);
    gnu_hash_table_t * gnu_hash = dyn_get_gnu_hash(dynamic, base);
    if (elf_hash == NULL && gnu_hash == NULL) {
        ERROR(elf, "ELF has no symbol hash table\n");
        return false;
    }

    return true;
}

Elf64_Phdr * phdr_get_ent(Elf64_Phdr * phdr, size_t phdrs, uint32_t type) {
    for (size_t i = 0; i < phdrs; i++) {
        if (phdr[i].p_type == type) {
            return &phdr[i];
        }
    }
    return NULL;
}

Elf64_Dyn * phdr_get_dyn(Elf64_Phdr * phdr, size_t phdrs, char * base) {
    Elf64_Phdr * ent = phdr_get_ent(phdr, phdrs, PT_DYNAMIC);
    if (ent == NULL) return NULL;
    return (Elf64_Dyn *)(base + ent->p_vaddr);
}

Elf64_Dyn * dyn_get_ent(Elf64_Dyn * dyn, int64_t tag) {
    for (size_t i = 0; dyn[i].d_tag != DT_NULL; i++) {
        if (dyn[i].d_tag == tag) {
            return &dyn[i];
        }
    }
    return NULL;
}

Elf64_Sym * dyn_get_symtab(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_SYMTAB);
    if (ent == NULL) return NULL;
    return (Elf64_Sym *)(base + ent->d_un.d_ptr);
}

elf_hash_table_t * dyn_get_elf_hash(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_HASH);
    if (ent == NULL) return NULL;
    return (elf_hash_table_t *)(base + ent->d_un.d_ptr);
}

gnu_hash_table_t * dyn_get_gnu_hash(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_GNU_HASH);
    if (ent == NULL) return NULL;
    return (gnu_hash_table_t *)(base + ent->d_un.d_ptr);
}

char * dyn_get_strtab(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_STRTAB);
    if (ent == NULL) return NULL;
    return base + ent->d_un.d_ptr;
}

Elf64_Rela * dyn_get_rela(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_RELA);
    if (ent == NULL) return NULL;
    return (Elf64_Rela *)(base + ent->d_un.d_ptr);
}

size_t dyn_get_rela_length(Elf64_Dyn * dyn) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_RELASZ);
    if (ent == NULL) return 0;
    return ent->d_un.d_val / sizeof (Elf64_Rela);
}

Elf64_Rela * dyn_get_pltrel(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_JMPREL);
    if (ent == NULL) return NULL;
    return (Elf64_Rela *)(base + ent->d_un.d_ptr);
}

size_t dyn_get_pltrel_length(Elf64_Dyn * dyn) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_PLTRELSZ);
    if (ent == NULL) return 0;
    return ent->d_un.d_val / sizeof (Elf64_Rela);
}

char * dyn_get_pltgot(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_PLTGOT);
    if (ent == NULL) return NULL;
    return base + ent->d_un.d_ptr;
}

char * dyn_get_rpath(Elf64_Dyn * dyn, char * strtab) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_RPATH);
    if (ent == NULL) return NULL;
    return strtab + ent->d_un.d_val;
}

char * dyn_get_runpath(Elf64_Dyn * dyn, char * strtab) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_RUNPATH);
    if (ent == NULL) return NULL;
    return strtab + ent->d_un.d_val;
}

voidfn_t * dyn_get_init(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_INIT);
    if (ent == NULL) return NULL;
    return (voidfn_t *)(base + ent->d_un.d_ptr);
}

voidfn_t ** dyn_get_init_array(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_INIT_ARRAY);
    if (ent == NULL) return NULL;
    return (voidfn_t **)(base + ent->d_un.d_ptr);
}

size_t dyn_get_init_array_length(Elf64_Dyn * dyn) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_INIT_ARRAYSZ);
    if (ent == NULL) return 0;
    return ent->d_un.d_val / sizeof (voidfn_t *);
}

voidfn_t * dyn_get_fini(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_FINI);
    if (ent == NULL) return NULL;
    return (voidfn_t *)(base + ent->d_un.d_ptr);
}

voidfn_t ** dyn_get_fini_array(Elf64_Dyn * dyn, char * base) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_FINI_ARRAY);
    if (ent == NULL) return NULL;
    return (voidfn_t **)(base + ent->d_un.d_ptr);
}

size_t dyn_get_fini_array_length(Elf64_Dyn * dyn) {
    Elf64_Dyn * ent = dyn_get_ent(dyn, DT_FINI_ARRAYSZ);
    if (ent == NULL) return 0;
    return ent->d_un.d_val / sizeof (voidfn_t *);
}
