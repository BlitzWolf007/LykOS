#include "sys/proc.h"
#include "arch/misc.h"
#include "arch/timer.h"
#include "log.h"
#include "mm/vm.h"
#include "sys/elf.h"
#include "sys/sched.h"
#include "sys/smp.h"
#include "sys/syscall.h"
#include "sys/thread.h"
#include "uapi/errno.h"
#include "utils/math.h"
#include "utils/string.h"
#include <stddef.h>
#include <stdint.h>

sys_ret_t syscall_execve(const char *path, const char *const argv[], const char *const envp[])
{
    if (!path || !argv || !envp)
        return (sys_ret_t) {0, EINVAL};

    // char _path[64];
    // vm_copy_from_user(sys_curr_as(), _path, (uint64_t)path, 64);
    const char *_argv[] = {NULL};
    const char *_envp[] = {NULL};

    return (sys_ret_t) {0, proc_execve(sys_curr_proc(), "/usr/bin/console", _argv, _envp)};
}

sys_ret_t syscall_exit(int code)
{
    log(LOG_DEBUG, "Process exited with code: %i.", code);

    sched_yield(THREAD_STATUS_TERMINATED);

    unreachable();
}

sys_ret_t syscall_fork()
{
    proc_t *child_proc = proc_fork(sys_curr_proc(), sys_curr_thread());

    list_node_t *n = child_proc->threads.head;
    thread_t *t = LIST_GET_CONTAINER(n, thread_t, proc_thread_list_node);
    sched_enqueue(t);

    return (sys_ret_t) {child_proc->pid, EOK};
}

sys_ret_t syscall_get_cwd(char *buffer, size_t size)
{
    vm_copy_to_user(sys_curr_as(), (uintptr_t)buffer, sys_curr_proc()->cwd,
                    MIN(strlen(sys_curr_proc()->cwd) + 1, size));

    return (sys_ret_t) {(uint64_t)buffer, EOK};
}

sys_ret_t syscall_get_pid()
{
    return (sys_ret_t) {sys_curr_proc()->pid, EOK};
}

sys_ret_t syscall_get_ppid()
{
    return (sys_ret_t) {sys_curr_proc()->parent ? sys_curr_proc()->parent->pid : 0, EOK};
}

sys_ret_t syscall_get_tid()
{
    return (sys_ret_t) {sys_curr_thread()->tid, EOK};
}

sys_ret_t syscall_tcb_set(void *ptr)
{
    arch_syscall_tcb_set(ptr);

    return (sys_ret_t) {0, EOK};
}

// sleep in microseconds
sys_ret_t syscall_sleep(unsigned us)
{
    sys_curr_thread()->sleep_until = arch_timer_get_uptime_ns() + (uint64_t) us * 1000;
    sched_yield(THREAD_STATUS_SLEEPING);
    return (sys_ret_t) {0, EOK};
}
