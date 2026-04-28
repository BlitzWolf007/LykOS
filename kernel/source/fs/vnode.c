#include "arch/types.h"
#include "fs/vfs.h"

#include "arch/clock.h"
#include "assert.h"
#include "hhdm.h"
#include "mm/mm.h"
#include "mm/pm.h"
#include "mm/vm.h"
#include "mm/vm/vm_object.h"
#include "uapi/errno.h"
#include "utils/math.h"
#include "utils/xarray.h"

static int get_page(vnode_t *vn, uint64_t pg_idx, bool read, page_t **out)
{
    page_t *page = xa_get(&vn->cached_pages, pg_idx);
    if (page)
    {
        *out = page;
        return EOK;
    }

    page = pm_alloc(0);
    if (!page)
        return ENOMEM;

    if (read)
    {
        uint64_t read_bytes;
        int err = vn->ops->read(
            vn,
            (void *)(page->addr + HHDM),
            pg_idx * ARCH_PAGE_GRAN,
            ARCH_PAGE_GRAN,
            &read_bytes
        );
        if (err != EOK)
        {
            pm_free(page);
            return err;
        }
    }

    if(!xa_insert(&vn->cached_pages, pg_idx, page))
        return ENOMEM;
    *out = page;
    return EOK;
}

int vnode_read(vnode_t *vn, void *buffer, uint64_t offset, uint64_t count,
               uint64_t *out_bytes_read)
{
    ASSERT(vn && buffer && out_bytes_read);

    if (!vn->ops->read)
        return ENOTSUP;

    if (offset >= vn->size)
    {
        *out_bytes_read = 0;
        return EOK;
    }

    uint64_t total_read = 0;
    while (total_read < count)
    {
        uint64_t pos     = offset + total_read;
        uint64_t pg_idx  = pos / ARCH_PAGE_GRAN;
        uint64_t pg_off  = pos % ARCH_PAGE_GRAN;
        uint64_t to_copy = MIN(ARCH_PAGE_GRAN - pg_off, count - total_read);

        page_t *page;
        int err = get_page(vn, pg_idx, true, &page);
        if (err != EOK)
            return err;

        memcpy(
            (uint8_t *)buffer + total_read,
            (uint8_t *)page->addr + HHDM + pg_off,
            to_copy
        );

        total_read += to_copy;
    }

    *out_bytes_read = total_read;
    return EOK;
}

int vnode_write(vnode_t *vn, void *buffer, uint64_t offset, uint64_t count,
                uint64_t *out_bytes_written)
{
    ASSERT(vn && buffer && out_bytes_written);

    if (!vn->ops->write)
        return ENOTSUP;

    uint64_t total_written = 0;
    while (total_written < count)
    {
        uint64_t pos     = offset + total_written;
        uint64_t pg_idx  = pos / ARCH_PAGE_GRAN;
        uint64_t pg_off  = pos % ARCH_PAGE_GRAN;
        uint64_t to_copy = MIN(ARCH_PAGE_GRAN - pg_off, count - total_written);

        page_t *page;
        // read-modify-write only if needed
        bool full_page = pg_off == 0 && to_copy == ARCH_PAGE_GRAN;
        int err = get_page(vn, pg_idx, full_page, &page);
        if (err != EOK)
            return err;

        memcpy(
            (uint8_t *)page->addr + HHDM + pg_off,
            (uint8_t *)buffer + total_written,
            to_copy
        );

        // Mark dirty.
        xa_set_mark(&vn->cached_pages, pg_idx, XA_MARK_0);
        total_written += to_copy;
    }

    if (offset + total_written > vn->size)
        vn->size = offset + total_written;
    *out_bytes_written = total_written;
    return EOK;
}

int vnode_ioctl(vnode_t *vn, uint64_t cmd, void *args)
{
    ASSERT(vn); // args can be NULL

    if (!vn->ops->ioctl)
        return ENOTSUP;

    return vn->ops->ioctl(vn, cmd, args);
}
