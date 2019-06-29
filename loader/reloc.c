#include "reloc.h"
#include "sym.h"
#include "elf.h"
#include "debug.h"

static bool dso_relocate_rela(dso_t * dso, Elf64_Rela * rela, size_t rela_length) {
    Elf64_Sym * symtab = dyn_get_symtab(dso->dyn, dso->base);
    char * strtab = dyn_get_strtab(dso->dyn, dso->base);

    for (size_t i = 0; i < rela_length; i++) {
        Elf64_Rela * cur = &rela[i];
        size_t type = ELF64_R_TYPE(cur->r_info);
        size_t sym_idx = ELF64_R_SYM(cur->r_info);

        Elf64_Sym * cur_sym = NULL;
        size_t cur_sym_val = 0;
        if (sym_idx != STN_UNDEF) {
            cur_sym = &symtab[sym_idx];
            size_t sym_bind = ELF64_ST_BIND(cur_sym->st_info);

            if (cur_sym->st_shndx == SHN_UNDEF) {
                if (cur_sym->st_name == 0) {
                    ERROR(reloc, "Cannot resolve symbol without name\n");
                    return false;
                }
                char * sym_name = &strtab[cur_sym->st_name];
                void * sym = resolve_symbol(sym_name);
                if (sym_bind != STB_WEAK && sym == NULL) {
                    ERROR(reloc, "Unresolved symbol '%s'\n", sym_name);
                    return false;
                }

                cur_sym_val = (size_t)sym;
            } else {
                cur_sym_val = (size_t)dso->base + cur_sym->st_value;
            }
        }

        size_t target = (size_t)(dso->base + cur->r_offset);

        switch (type) {
            case R_X86_64_NONE:
                return false;
            case R_X86_64_64:
                *(uint64_t *)target = cur_sym_val + cur->r_addend;
                break;
            case R_X86_64_PC32:
                *(uint32_t *)target = cur_sym_val + cur->r_addend - target;
                break;
            case R_X86_64_COPY:
                ERROR(reloc, "Don't know how to relocate R_X86_64_COPY\n");
                return false;
            case R_X86_64_GLOB_DAT:
                *(uint64_t *)target = cur_sym_val;
                break;
            case R_X86_64_JUMP_SLOT:
                *(uint64_t *)target = cur_sym_val;
                break;
            case R_X86_64_RELATIVE:
                *(uint64_t *)target = (size_t)dso->base + cur->r_addend;
                break;
            case R_X86_64_GOTPCREL:
                ERROR(reloc, "Don't know how to relocate R_X86_64_GOTPCREL\n");
                return false;
            case R_X86_64_32:
            case R_X86_64_32S:
                *(uint32_t *)target = cur_sym_val + cur->r_addend;
                break;
            case R_X86_64_16:
                *(uint16_t *)target = cur_sym_val + cur->r_addend;
                break;
            case R_X86_64_8:
                *(uint8_t *)target = cur_sym_val + cur->r_addend;
                break;
            default:
                ERROR(reloc, "Unknown Relocation Type: %zd\n", type);
                return false;
        }
    }

    return true;
}

bool dso_relocate(dso_t * dso) {
    DEBUG(reloc, "Relocating '%s'\n", dso->path);

    Elf64_Rela * rela = dyn_get_rela(dso->dyn, dso->base);
    size_t rela_length = dyn_get_rela_length(dso->dyn);
    if (rela != NULL) {
        DEBUG(reloc, "Applying RELA relocations\n");
        if (!dso_relocate_rela(dso, rela, rela_length)) return false;
    }

    Elf64_Rela * pltrel = dyn_get_pltrel(dso->dyn, dso->base);
    size_t pltrel_length = dyn_get_pltrel_length(dso->dyn);
    if (pltrel != NULL) {
        DEBUG(reloc, "Applying PLTREL relocations\n");
        if (!dso_relocate_rela(dso, pltrel, pltrel_length)) return false;
    }

    return true;
}
