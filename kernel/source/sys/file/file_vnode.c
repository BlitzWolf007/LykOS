#include "arch/types.h"
#include "fs/vfs.h"
#include "mm/mm.h"
#include "mm/vm.h"
#include "sys/file.h"
#include "sys/poll.h"
#include "sys/uio.h"
#include "sys/unistd.h"
#include "uapi/errno.h"
#include <stdint.h>

static int file_vnode_read(file_t *fp, uio_op_t *uio,
                           [[maybe_unused]] int flags,
                           [[maybe_unused]] thread_t *td)
{
    vnode_t *vn = fp->backend;
    int error = EOK;

    for (size_t i = 0; i < uio->buf_cnt && uio->rem_bytes > 0; i++)
    {
        uio_op_buf_t *b = &uio->buf[i];

        if (b->length == 0)
            continue;

        uint64_t to_read = b->length;
        if (to_read > (uint64_t)uio->rem_bytes)
            to_read = uio->rem_bytes;

        uint64_t done = 0;

        error = vnode_read(
            vn,
            b->base,
            fp->offset,
            to_read,
            &done
        );
        if (error)
            return error;

        fp->offset     += done;
        uio->offset    += done;
        uio->rem_bytes -= done;

        if (done < to_read)
            break; // EOF
    }

    return EOK;
}

static int file_vnode_write(file_t *fp, uio_op_t *uio,
                            [[maybe_unused]] int flags,
                            [[maybe_unused]] thread_t *td)
{
    vnode_t *vn = fp->backend;
    int error = EOK;

    uint64_t offset = fp->offset;

    if (fp->flags & O_APPEND)
        offset = vn->size;

    for (size_t i = 0; i < uio->buf_cnt && uio->rem_bytes > 0; i++)
    {
        uio_op_buf_t *b = &uio->buf[i];

        if (b->length == 0)
            continue;

        uint64_t to_write = b->length;
        if (to_write > (uint64_t)uio->rem_bytes)
            to_write = uio->rem_bytes;

        uint64_t done = 0;

        error = vnode_write(
            vn,
            b->base,
            offset,
            to_write,
            &done
        );
        if (error)
            return error;

        offset          += done;
        uio->offset     += done;
        uio->rem_bytes  -= done;

        if (done < to_write)
            break;
    }

    fp->offset = offset;

    return EOK;
}

static int file_vnode_ioctl(file_t *fp, int cmd, void *data, [[maybe_unused]] thread_t *td)
{
    vnode_t *vn = fp->backend;

    if (!vn->ops->ioctl)
        return EINVAL;

    return vn->ops->ioctl(vn, cmd, data);
}

static int file_vnode_poll([[maybe_unused]] file_t *fp, int events,
                           [[maybe_unused]] thread_t *td)
{
    return events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM);
}

static int file_vnode_truncate([[maybe_unused]] file_t *fp,
                               [[maybe_unused]] off_t length,
                               [[maybe_unused]] thread_t *td)
{
    return ENOTSUP;
}

static int file_vnode_chmod([[maybe_unused]] file_t *fp,
                            [[maybe_unused]] mode_t mode,
                            [[maybe_unused]] thread_t *td)
{
    return ENOTSUP;
}

static int file_vnode_chown([[maybe_unused]] file_t *fp,
                            [[maybe_unused]] uid_t uid,
                            [[maybe_unused]] gid_t gid,
                            [[maybe_unused]] thread_t *td)
{
    return ENOTSUP;
}

static int file_vnode_seek(file_t *fp, off_t offset, int whence)
{
    vnode_t *vn = fp->backend;
    off_t newoff;

    switch (whence)
    {
    case SEEK_SET:
        newoff = offset;
        break;
    case SEEK_CUR:
        newoff = fp->offset + offset;
        break;
    case SEEK_END:
        newoff = vn->size + offset;
        break;
    default:
        return EINVAL;
    }

    if (newoff < 0)
        return EINVAL;

    fp->offset = newoff;
    return EOK;
}

static int file_vnode_mmap(file_t *fp, vm_addrspace_t *as, uintptr_t vaddr,
                           size_t length, int prot, int flags, off_t offset,
                           [[maybe_unused]] thread_t *td, uintptr_t *out_vaddr)
{
    vnode_t *vn = fp->backend;

    if (length == 0 || offset < 0)
        return EINVAL;
    if ((uintptr_t)offset & (ARCH_PAGE_GRAN - 1))
        return EINVAL;
    if ((prot & VM_PROTECTION_WRITE) && !(fp->flags & O_WRONLY) && !(fp->flags & O_RDWR))
        return EACCES;
    if ((prot & VM_PROTECTION_READ) && !(fp->flags & O_RDONLY) && !(fp->flags & O_RDWR))
        return EACCES;

    int err = vnode_mmap(vn, as, vaddr, length, prot, flags, offset, out_vaddr);
    if (err != EOK)
        return err;

    return EOK;
}

static int file_vnode_close(file_t *fp, [[maybe_unused]] thread_t *td)
{
    vnode_t *vn = fp->backend;
    vnode_unref(vn);

    return EOK;
}

const file_ops_t file_vnode_ops = {
    .read     = file_vnode_read,
    .write    = file_vnode_write,
    .ioctl    = file_vnode_ioctl,
    .poll     = file_vnode_poll,
    .truncate = file_vnode_truncate,
    .chmod    = file_vnode_chmod,
    .chown    = file_vnode_chown,
    .seek     = file_vnode_seek,
    .mmap     = file_vnode_mmap,
    .close    = file_vnode_close,
};
