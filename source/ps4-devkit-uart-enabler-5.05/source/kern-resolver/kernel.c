/*
 * ps4-kexec - a kexec() implementation for Orbis OS / FreeBSD
 *
 * Copyright (C) 2015-2016 shuffle2 <godisgovernment@gmail.com>
 * Copyright (C) 2015-2016 Hector Martin "marcan" <marcan@marcan.st>
 *
 * This code is licensed to you under the 2-clause BSD license. See the LICENSE
 * file for more information.
 */

#include "kernel.h"
#include "ps4.h"
#include "elf.h"
#include "../define.h"


//#define KERNBASE    0xffffffff80000000ull
#define KERNSIZE    0x2000000

static const u8 ELF_IDENT[9] = "\x7f" "ELF\x02\x01\x01\x09\x00";
static Elf64_Sym *symtab;
static char *strtab;
static size_t strtab_size;

static void* KERNBASE = 0;

static Elf64_Ehdr *find_kern_ehdr(void)
{
    // Search for the kernel copy embedded in ubios, then follow it to see
    // where it was relocated to
    for (void* p = KERNBASE; p < KERNBASE + KERNSIZE; p += PAGE_SIZE) {
        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)p;
        if (!memcmp(ehdr->e_ident, ELF_IDENT, sizeof(ELF_IDENT))) {
            for (size_t i = 0; i < ehdr->e_phnum; i++) {
                Elf64_Phdr *phdr = (Elf64_Phdr *)(p + ehdr->e_phoff) + i;
                if (phdr->p_type == PT_PHDR) {
                    return (Elf64_Ehdr *)(phdr->p_vaddr - ehdr->e_phoff);
                }
            }
        }
    }
    return NULL;
}

static Elf64_Dyn *elf_get_dyn(Elf64_Ehdr *ehdr)
{
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)ehdr + ehdr->e_phoff);
    for (size_t i = 0; i < ehdr->e_phnum; i++, phdr++) {
        if (phdr->p_type == PT_DYNAMIC) {
            return (Elf64_Dyn *)phdr->p_vaddr;
        }
    }
    return NULL;
}

static int elf_parse_dyn(Elf64_Dyn *dyn)
{
    for (Elf64_Dyn *dp = dyn; dp->d_tag != DT_NULL; dp++) {
        switch (dp->d_tag) {
            case DT_SYMTAB:
                symtab = (Elf64_Sym *)dp->d_un.d_ptr;
                break;
            case DT_STRTAB:
                strtab = (char *)dp->d_un.d_ptr;
                break;
            case DT_STRSZ:
                strtab_size = dp->d_un.d_val;
                break;
        }
    }
    return symtab && strtab && strtab_size;
}

void *kernel_resolve(void *td, const char *name)
{
    for (Elf64_Sym *sym = symtab; (uintptr_t)(sym + 1) < (uintptr_t)strtab; sym++) {
        if (!strcmp(name, &strtab[sym->st_name])) {
            kprintf("kern.%s = %p\n", name, (void*)sym->st_value);
            return (void *)sym->st_value;
        }
    }
    kprintf("Failed to resolve symbol '%s'\n", name);
    return NULL;
}

int kernel_init(void *td, void* kernbase)
{
    kprintf("kernel_init()\n");

    KERNBASE = kernbase;

    Elf64_Ehdr *ehdr = find_kern_ehdr();
    if (!ehdr) {
        kprintf("Could not find kernel ELF header\n");
        goto err;
    }
    kprintf("ELF header at %p\n", ehdr);

    Elf64_Dyn *dyn = elf_get_dyn(ehdr);
    if (!dyn) {
        kprintf("Could not find kernel dynamic header\n");
        goto err;
    }
    kprintf("ELF dynamic section at %p\n", dyn);

    if (!elf_parse_dyn(dyn)) {
        kprintf("Failed to parse ELF dynamic section\n");
        goto err;
    }

    return 0;
err:
   
    return -1;
}
