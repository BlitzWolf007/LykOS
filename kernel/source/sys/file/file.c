#include "sys/file.h"

#include "assert.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "uapi/errno.h"
#include <stdint.h>

extern const file_ops_t file_vnode_ops;
extern const file_ops_t file_socket_ops;
extern const file_ops_t file_pipe_vnode;
extern const file_ops_t file_fifo_vnode;
extern const file_ops_t file_msgq_vnode;
extern const file_ops_t file_eventq_ops;

/*
 * Creation
 */

file_t *file_alloc(file_type_t type, const file_ops_t *ops, void *backend,
                   int flags)
{
    ASSERT(ops);

    file_t *file = heap_alloc(sizeof(file_t));
    if (!file)
        return NULL;
    *file = (file_t) {
        .type = type,
        .ops = ops,
        .backend = backend,
        .flags = flags,
        .offset = 0,
        .refcount = REF_INIT,
        .slock = SPINLOCK_INIT
    };

    return file;
}

file_t *file_create_vnode(vnode_t *vn, int flags)
{
    ASSERT(vn);

    vnode_ref(vn);

    file_t *f = file_alloc(
        FILE_TYPE_VNODE,
        &file_vnode_ops,
        vn,
        flags
    );
    if (!f)
        vnode_unref(vn);

    return f;
}

file_t *file_create_socket([[maybe_unused]] socket_t *so,
                           [[maybe_unused]] int flags)
{
    ASSERT(so);

    file_t *f = file_alloc(
        FILE_TYPE_SOCKET,
        &file_socket_ops,
        so,
        flags
    );

    return f;
}

file_t *file_create_pipe([[maybe_unused]] pipe_t *pipe,
                         [[maybe_unused]] int flags,
                         [[maybe_unused]] bool writable_end)
{
    return NULL;
}

file_t *file_create_fifo([[maybe_unused]] fifo_t *vn,
                         [[maybe_unused]] int flags)
{
    return NULL;
}

file_t *file_create_msgq([[maybe_unused]] msgq_t *mq,
                         [[maybe_unused]] int flags)
{
    return NULL;
}

file_t *file_create_eventq([[maybe_unused]] eventq_t *eq,
                           [[maybe_unused]] int flags)
{
    return NULL;
}

/*
 * Wrappers
 */

int file_read(file_t *fp, uio_op_t *uio_op, int flags, [[maybe_unused]] thread_t *td)
{
    if (!fp->ops->read)
        return ENOTSUP;

    return fp->ops->read(fp, uio_op, flags, td);
}

int file_write(file_t *fp, uio_op_t *uio_op, int flags, [[maybe_unused]] thread_t *td)
{
    if (!fp->ops->write)
        return ENOTSUP;

    return fp->ops->write(fp, uio_op, flags, td);
}

int file_ioctl(file_t *fp, int cmd, void *data, [[maybe_unused]] thread_t *td)
{
    if (!fp->ops->ioctl)
        return ENOTTY;

    return fp->ops->ioctl(fp, cmd, data, td);
}

int file_poll(file_t *fp, int events, [[maybe_unused]] thread_t *td)
{
    if (!fp->ops->poll)
        return ENOTSUP;

    return fp->ops->poll(fp, events, td);
}

int file_truncate(file_t *fp, off_t length, [[maybe_unused]] thread_t *td)
{
    if (!fp->ops->truncate)
        return ENOTSUP;

    return fp->ops->truncate(fp, length, td);
}

int file_chmod(file_t *fp, mode_t mode, [[maybe_unused]] thread_t *td)
{
    if (!fp->ops->chmod)
        return ENOTSUP;

    return fp->ops->chmod(fp, mode, td);
}

int file_chown(file_t *fp, uid_t uid, gid_t gid, [[maybe_unused]] thread_t *td)
{
    if (!fp->ops->chown)
        return ENOTSUP;

    return fp->ops->chown(fp, uid, gid, td);
}

int file_seek(file_t *fp, off_t offset, int whence)
{
    if (!fp->ops->seek)
        return ESPIPE;

    return fp->ops->seek(fp, offset, whence);
}

int file_mmap(file_t *fp, vm_addrspace_t *as, uintptr_t vaddr, size_t length, int prot, int flags,
              off_t offset, thread_t *td, uintptr_t *out_vaddr)
{
    if (fp->ops->mmap)
        return ENODEV;

    return fp->ops->mmap(fp, as, vaddr, length, prot, flags, offset, td, out_vaddr);
}

int file_close(file_t *fp, [[maybe_unused]] thread_t *td)
{
    if (!fp->ops->close)
        return EOK;

    return fp->ops->close(fp, td);
}

/*
 * Ref counting
 */

void file_ref(file_t *file)
{
    ref_inc(&file->refcount);
}

void file_unref(file_t *file)
{
    if (ref_dec(&file->refcount))
        heap_free(file);
}
