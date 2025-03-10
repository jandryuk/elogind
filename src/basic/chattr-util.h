/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <linux/fs.h>
#include <stdbool.h>
#include <stddef.h>

#include "missing_fs.h"

/* The chattr() flags to apply when creating a new file *before* writing to it. In particular, flags such as
 * FS_NOCOW_FL don't work if applied a-posteriori. All other flags are fine (or even necessary, think
 * FS_IMMUTABLE_FL!) to apply after writing to the files. */
#define CHATTR_EARLY_FL                         \
        (FS_NOATIME_FL |                        \
         FS_COMPR_FL   |                        \
         FS_NOCOW_FL   |                        \
         FS_NOCOMP_FL  |                        \
         FS_PROJINHERIT_FL)

#define CHATTR_ALL_FL                           \
        (FS_NOATIME_FL      |                   \
         FS_SYNC_FL         |                   \
         FS_DIRSYNC_FL      |                   \
         FS_APPEND_FL       |                   \
         FS_COMPR_FL        |                   \
         FS_NODUMP_FL       |                   \
         FS_EXTENT_FL       |                   \
         FS_IMMUTABLE_FL    |                   \
         FS_JOURNAL_DATA_FL |                   \
         FS_SECRM_FL        |                   \
         FS_UNRM_FL         |                   \
         FS_NOTAIL_FL       |                   \
         FS_TOPDIR_FL       |                   \
         FS_NOCOW_FL        |                   \
         FS_PROJINHERIT_FL)

int chattr_full(const char *path, int fd, unsigned value, unsigned mask, unsigned *ret_previous, unsigned *ret_final, bool fallback);
static inline int chattr_fd(int fd, unsigned value, unsigned mask, unsigned *previous) {
        return chattr_full(NULL, fd, value, mask, previous, NULL, false);
}
static inline int chattr_path(const char *path, unsigned value, unsigned mask, unsigned *previous) {
        return chattr_full(path, -1, value, mask, previous, NULL, false);
}

#if 0 /// UNNEEDED by elogind
int read_attr_fd(int fd, unsigned *ret);
int read_attr_path(const char *p, unsigned *ret);
#endif // 0
