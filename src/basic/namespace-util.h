/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <sys/types.h>

int namespace_open(pid_t pid, int *pidns_fd, int *mntns_fd, int *netns_fd, int *userns_fd, int *root_fd);
int namespace_enter(int pidns_fd, int mntns_fd, int netns_fd, int userns_fd, int root_fd);

#if 0 /// UNNEEDED by elogind
#endif // 0
int fd_is_ns(int fd, unsigned long nsflag);

int detach_mount_namespace(void);
