#pragma once

#include "fs/vfs.h"
#include "sys/proc.h"

int elf_load(vnode_t *, proc_t **out_proc);
