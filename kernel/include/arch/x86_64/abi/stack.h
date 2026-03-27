#include "mm/vm.h"
#include <stdint.h>

/*
 * @brief Allocate and initialize stack for a new thread.
 *
 * If @p argv and @p envp are provided, the stack is initialized with
 * `argc`, `argv[]`, and `envp[]` as expected for a process entry point.
 * If both are NULL, a minimal stack suitable for a regular thread is created.
 *
 * @param out_sp Initial stack pointer (RSP)
 *
 * @return EOK on success, or an error code on failure.
 */
int x86_64_abi_stack_setup(vm_addrspace_t *as, size_t stack_size, char **argv, char **envp, uint64_t *out_sp);
