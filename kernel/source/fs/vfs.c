#include "fs/vfs.h"

#include "arch/types.h"
#include "assert.h"
#include "fs/mount.h"
#include "fs/path.h"
#include "fs/ramfs.h"
#include "hhdm.h"
#include "log.h"
#include "mm/mm.h"
#include "mm/pm.h"
#include "uapi/errno.h"
#include "utils/list.h"
#include "utils/math.h"
#include "utils/string.h"

void vnode_ref(vnode_t *vn)
{
    ref_inc(&vn->refcount);
}

void vnode_unref(vnode_t *vn)
{
    if (ref_dec(&vn->refcount))
    {
        // TODO: run vnode destructor
    }
}

/*
 * Veneer layer.
 */

int vfs_lookup(const char *path, vnode_t **out_vn)
{
    ASSERT(path_is_absolute(path));

    vfsmount_t *vfsmount = find_mount(path, &path);
    vnode_t *curr = vfsmount->vfs->vfs_ops->get_root(vfsmount->vfs);

    char comp[PATH_MAX + 1];
    size_t comp_len;
    while (curr)
    {
        if (!*path)
            break;

        path = path_next_component(path, comp, &comp_len);

        if (curr->ops->lookup(curr, comp, &curr) != EOK)
            return ENOENT;
    }

    *out_vn = curr;
    return EOK;
}

int vfs_create(const char *path, vnode_type_t type, vnode_t **out)
{
    ASSERT(path_is_absolute(path));

    char dirname[PATH_MAX];
    char basename[PATH_MAX];
    size_t dirname_len;
    size_t basename_len;

    path_split(path, dirname, &dirname_len, basename, &basename_len);

    vnode_t *parent;
    int ret = vfs_lookup(dirname, &parent);
    if (ret != EOK)
        return ret;

    return parent->ops->create(parent, basename, type, out);
}

int vfs_remove(const char *path)
{
    ASSERT(path_is_absolute(path));

    char dirname[PATH_MAX];
    char basename[PATH_MAX];
    size_t dirname_len;
    size_t basename_len;

    path_split(path, dirname, &dirname_len, basename, &basename_len);

    vnode_t *parent;
    int ret = vfs_lookup(dirname, &parent);
    if (ret != EOK)
        return ret;

    return parent->ops->remove(parent, basename);
}

/*
 * Initialization
 */

void vfs_init()
{
    vfs_t *ramfs = ramfs_create();
    if (!ramfs)
        panic("Failed to crate root ramfs!");

    mount_init(ramfs);

    log(LOG_INFO, "VFS initialized.");
}
