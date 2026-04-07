#pragma once

#include "sys/proc.h"

int elf_load(proc_t *proc, const char *path, const char *const argv[], const char *const envp[]);
