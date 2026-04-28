#include "sys/elf.h"

#include "arch/types.h"
#include "assert.h"
#include "fs/vfs.h"
#include "log.h"
#include "mm/heap.h"
#include "mm/mm.h"
#include "mm/vm.h"
#include "sys/proc.h"
#include "sys/thread.h"
#include "uapi/errno.h"
#include "utils/elf.h"
#include "utils/list.h"
#include "utils/math.h"
#include <stddef.h>
#include <stdint.h>

static int elf_load_ph(vm_addrspace_t *as, vnode_t *file, Elf64_Phdr *ph)
{
    int err = EOK;

    if (ph->p_filesz > ph->p_memsz)
        return ENOEXEC;

    uintptr_t seg_end;
    if (__builtin_add_overflow(ph->p_vaddr, ph->p_memsz, &seg_end))
        return ENOEXEC;

    uintptr_t start = FLOOR(ph->p_vaddr, ARCH_PAGE_GRAN);
    uintptr_t end   = CEIL(seg_end, ARCH_PAGE_GRAN);

    uintptr_t diff;
    if (__builtin_sub_overflow(end, start, &diff))
        return ENOEXEC;

    vm_protection_t prot = 0;
    if (ph->p_flags & PF_R) prot |= VM_PROTECTION_READ;
    if (ph->p_flags & PF_W) prot |= VM_PROTECTION_WRITE;
    if (ph->p_flags & PF_X) prot |= VM_PROTECTION_EXECUTE;

    uintptr_t out;
    err = vm_map(
        as,
        start, diff,
        prot,
        VM_MAP_ANON | VM_MAP_FIXED | VM_MAP_PRIVATE | VM_MAP_POPULATE,
        NULL, 0,
        &out
    );
    if (err != EOK || out != start)
        return ENOEXEC;

    if (ph->p_filesz == 0)
        return EOK;

    CLEANUP uint8_t *buf = heap_alloc(1024);
    if (!buf)
        return ENOMEM;

    size_t read_bytes = 0;
    while (read_bytes < ph->p_filesz)
    {
        size_t to_copy = MIN(ph->p_filesz - read_bytes, 1024);
        uintptr_t off;
        if (__builtin_add_overflow(ph->p_offset, read_bytes, &off))
            return ENOEXEC;

        uint64_t count;
        if (vnode_read(file, buf, off, to_copy, &count) != EOK
        ||  count != to_copy)
            return ENOEXEC;
        vm_copy_to_user(as, ph->p_vaddr + read_bytes, buf, to_copy);

        read_bytes += to_copy;
    }

    return EOK;
}

int elf_load(vm_addrspace_t *as, const char *path, void **out_entry, char **out_interpreter)
{
    ASSERT(as && as->segments.length == 0);
    *out_entry = NULL;
    *out_interpreter = NULL;

    log(LOG_INFO, "Loading ELF...");

    int err = EOK;
    uint64_t count;

    vnode_t *file;
    err = vfs_lookup(path, &file);
    if (err != EOK)
        return err;

    Elf64_Ehdr ehdr;
    err = vnode_read(file, &ehdr, 0, sizeof(Elf64_Ehdr), &count);
    if (err != EOK || count != sizeof(Elf64_Ehdr))
        return err;

    if (memcmp(ehdr.e_ident, "\x7F""ELF", 4)
    ||  ehdr.e_ident[EI_CLASS]   != ELFCLASS64
    ||  ehdr.e_ident[EI_DATA]    != ELFDATA2LSB
    #if defined(__x86_64__)
    ||  ehdr.e_machine           != EM_x86_64
    #elif defined(__aarch64__)
    ||  ehdr.e_machine           != EM_AARCH64
    #endif
    ||  ehdr.e_ident[EI_VERSION] != EV_CURRENT
    ||  ehdr.e_type              != ET_EXEC)
        return ENOEXEC;

    Elf64_Phdr *ph_table = vm_alloc(ehdr.e_phentsize * ehdr.e_phnum);
    if (!ph_table)
        return ENOMEM;
    err = vnode_read(file, ph_table, ehdr.e_phoff, ehdr.e_phentsize * ehdr.e_phnum, &count);
    if (err != EOK || count != ehdr.e_phentsize * ehdr.e_phnum)
        return err;

    for (size_t i = 0; i < ehdr.e_phnum; i++)
    {
        Elf64_Phdr *ph = &ph_table[i];

        if (ph->p_type != PT_LOAD || ph->p_memsz == 0)
            continue;

        err = elf_load_ph(as, file, ph);
        if (err != EOK)
            goto fail;
    }

    *out_entry = (void *)ehdr.e_entry;
    // TODO: *out_interpreter =

    vm_free(ph_table);
    return EOK;

fail:
    // TODO: free loaded segments
    return err;
}
