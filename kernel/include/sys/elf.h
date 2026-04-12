#pragma once

#include "mm/vm.h"

int elf_load(vm_addrspace_t *as, const char *path, void **out_entry, char **out_interpreter);
