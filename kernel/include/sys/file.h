#pragma once

#include "fs/vfs.h"
#include "sync/spinlock.h"
#include "sys/types.h"
#include "utils/ref.h"

/*
 * Forward declarations
 */

typedef struct file file_t;
typedef struct file_ops file_ops_t;
typedef struct thread thread_t;
typedef struct uio_op uio_op_t;
typedef struct vnode vnode_t;

typedef struct socket socket_t;
typedef struct pipe pipe_t;
typedef struct fifo fifo_t;
typedef struct msgq msgq_t;
typedef struct eventq eventq_t;

/*
 * Open file types
 */

typedef enum file_type
{
    FILE_TYPE_VNODE,    // normal file from FS
    FILE_TYPE_SOCKET,
    FILE_TYPE_PIPE,
    FILE_TYPE_FIFO,     // named pipe
    FILE_TYPE_EVENT_QUEUE,
    FILE_TYPE_MESSAGE_QUEUE,
    FILE_TYPE_SHM       // shared memory
}
file_type_t;

/*
 * Open file structure
 */

struct file
{
    file_type_t type;
    const file_ops_t *ops;
    void *backend; // backing vnode/socket/etc.
    int flags;
    off_t offset;

    ref_t refcount;
    spinlock_t slock;
};

/*
 * Open file operations
 */

struct file_ops
{
    int (*read)     (file_t *fp, uio_op_t *uio_op, int flags, thread_t *td);
    int (*write)    (file_t *fp, uio_op_t *uio_op, int flags, thread_t *td);
    int (*ioctl)    (file_t *fp, int cmd, void *data, thread_t *td);
    int (*poll)     (file_t *fp, int events, thread_t *td);
    int (*truncate) (file_t *fp, off_t length, thread_t *td);
    int (*chmod)    (file_t *fp, mode_t mode, thread_t *td);
    int (*chown)    (file_t *fp, uid_t uid, gid_t gid, thread_t *td);
    int (*seek)     (file_t *fp, off_t offset, int whence);
    int (*mmap)     (file_t *fp, vm_addrspace_t *as, uintptr_t addr, size_t length,
                     int prot, int flags, off_t offset, thread_t *td,
                     uintptr_t *out_vaddr);
    int (*close)    (file_t *fp, thread_t *td);
};

/*
 * Public API
 */

file_t *file_create_vnode(vnode_t *vn, int flags);
file_t *file_create_socket(socket_t *so, int flags);
file_t *file_create_pipe(pipe_t *pipe, int flags, bool writable_end);
file_t *file_create_fifo(fifo_t *fifo, int flags);
file_t *file_create_eventq(eventq_t *eq, int flags);
file_t *file_create_msgq(msgq_t *mq, int flags);

int file_read(file_t *fp, uio_op_t *uio_op, int flags, thread_t *td);
int file_write(file_t *fp, uio_op_t *uio_op, int flags, thread_t *td);
int file_ioctl(file_t *fp, int cmd, void *data, thread_t *td);
int file_poll(file_t *fp, int events, thread_t *td);
int file_truncate(file_t *fp, off_t length, thread_t *td);
int file_chmod(file_t *fp, mode_t mode, thread_t *td);
int file_chown(file_t *fp, uid_t uid, gid_t gid, thread_t *td);
int file_seek(file_t *fp, off_t offset, int whence);
int file_mmap(file_t *fp, vm_addrspace_t *as, uintptr_t vaddr, size_t length,
              int prot, int flags, off_t offset, thread_t *td,
              uintptr_t *out_vaddr);
int file_close(file_t *fp, thread_t *td);

void file_ref(file_t *file);
void file_unref(file_t *file);
