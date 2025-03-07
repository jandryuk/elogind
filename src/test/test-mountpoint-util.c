/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <sched.h>
#include <sys/mount.h>
#include <unistd.h>

#include "alloc-util.h"
#include "def.h"
#include "fd-util.h"
#include "fileio.h"
#include "fs-util.h"
#include "hashmap.h"
#include "log.h"
#include "mkdir.h"
#include "mountpoint-util.h"
#include "path-util.h"
#include "rm-rf.h"
#include "string-util.h"
#include "tests.h"
/// Addition includes needed by elogind
#include "virt.h"
#include "tmpfile-util.h"

#if 0 /// UNNEEDED by elogind
static void test_mount_propagation_flags(const char *name, int ret, unsigned long expected) {
        long unsigned flags;

        log_info("/* %s(%s) */", __func__, name);

        assert_se(mount_propagation_flags_from_string(name, &flags) == ret);

        if (ret >= 0) {
                const char *c;

                assert_se(flags == expected);

                c = mount_propagation_flags_to_string(flags);
                if (isempty(name))
                        assert_se(isempty(c));
                else
                        assert_se(streq(c, name));
        }
}
#endif // 0

static void test_mnt_id(void) {
        _cleanup_fclose_ FILE *f = NULL;
        _cleanup_hashmap_free_free_ Hashmap *h = NULL;
        char *p;
        void *k;
        int r;

        log_info("/* %s */", __func__);

        assert_se(f = fopen("/proc/self/mountinfo", "re"));
        assert_se(h = hashmap_new(&trivial_hash_ops));

        for (;;) {
                _cleanup_free_ char *line = NULL, *path = NULL;
                int mnt_id;

                r = read_line(f, LONG_LINE_MAX, &line);
                if (r == 0)
                        break;
                assert_se(r > 0);

                assert_se(sscanf(line, "%i %*s %*s %*s %ms", &mnt_id, &path) == 2);
#if HAS_FEATURE_MEMORY_SANITIZER
                /* We don't know the length of the string, so we need to unpoison it one char at a time */
                for (const char *c = path; ;c++) {
                        msan_unpoison(c, 1);
                        if (!*c)
                                break;
                }
#endif
                log_debug("mountinfo: %s → %i", path, mnt_id);

                assert_se(hashmap_put(h, INT_TO_PTR(mnt_id), path) >= 0);
                path = NULL;
        }

        HASHMAP_FOREACH_KEY(p, k, h) {
                int mnt_id = PTR_TO_INT(k), mnt_id2;

                r = path_get_mnt_id(p, &mnt_id2);
                if (r < 0) {
                        log_debug_errno(r, "Failed to get the mnt id of %s: %m\n", p);
                        continue;
                }

                if (mnt_id == mnt_id2) {
                        log_debug("mnt ids of %s is %i\n", p, mnt_id);
                        continue;
                } else
                        log_debug("mnt ids of %s are %i, %i\n", p, mnt_id, mnt_id2);

                /* The ids don't match? If so, then there are two mounts on the same path, let's check if
                 * that's really the case */
                char *t = hashmap_get(h, INT_TO_PTR(mnt_id2));
                log_debug("the other path for mnt id %i is %s\n", mnt_id2, t);
#if 0 /// Unfortunately this doesn't work in all cases where elogind is running in a chroot. (#127)
                assert_se(path_equal(p, t));
#else // 0
                assert_se(path_equal(p, t) || (
                          running_in_chroot() && startswith(p, t)
                ) );
#endif // 0
        }
}

static void test_path_is_mount_point(void) {
        int fd;
        char tmp_dir[] = "/tmp/test-path-is-mount-point-XXXXXX";
        _cleanup_free_ char *file1 = NULL, *file2 = NULL, *link1 = NULL, *link2 = NULL;
        _cleanup_free_ char *dir1 = NULL, *dir1file = NULL, *dirlink1 = NULL, *dirlink1file = NULL;
        _cleanup_free_ char *dir2 = NULL, *dir2file = NULL;

        log_info("/* %s */", __func__);

        assert_se(path_is_mount_point("/", NULL, AT_SYMLINK_FOLLOW) > 0);
        assert_se(path_is_mount_point("/", NULL, 0) > 0);
        assert_se(path_is_mount_point("//", NULL, AT_SYMLINK_FOLLOW) > 0);
        assert_se(path_is_mount_point("//", NULL, 0) > 0);

        assert_se(path_is_mount_point("/proc", NULL, AT_SYMLINK_FOLLOW) > 0);
        assert_se(path_is_mount_point("/proc", NULL, 0) > 0);
        assert_se(path_is_mount_point("/proc/", NULL, AT_SYMLINK_FOLLOW) > 0);
        assert_se(path_is_mount_point("/proc/", NULL, 0) > 0);

        assert_se(path_is_mount_point("/proc/1", NULL, AT_SYMLINK_FOLLOW) == 0);
        assert_se(path_is_mount_point("/proc/1", NULL, 0) == 0);
        assert_se(path_is_mount_point("/proc/1/", NULL, AT_SYMLINK_FOLLOW) == 0);
        assert_se(path_is_mount_point("/proc/1/", NULL, 0) == 0);

        assert_se(path_is_mount_point("/sys", NULL, AT_SYMLINK_FOLLOW) > 0);
        assert_se(path_is_mount_point("/sys", NULL, 0) > 0);
        assert_se(path_is_mount_point("/sys/", NULL, AT_SYMLINK_FOLLOW) > 0);
        assert_se(path_is_mount_point("/sys/", NULL, 0) > 0);

        /* we'll create a hierarchy of different kinds of dir/file/link
         * layouts:
         *
         * <tmp>/file1, <tmp>/file2
         * <tmp>/link1 -> file1, <tmp>/link2 -> file2
         * <tmp>/dir1/
         * <tmp>/dir1/file
         * <tmp>/dirlink1 -> dir1
         * <tmp>/dirlink1file -> dirlink1/file
         * <tmp>/dir2/
         * <tmp>/dir2/file
         */

        /* file mountpoints */
        assert_se(mkdtemp(tmp_dir) != NULL);
        file1 = path_join(tmp_dir, "file1");
        assert_se(file1);
        file2 = path_join(tmp_dir, "file2");
        assert_se(file2);
        fd = open(file1, O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC, 0664);
        assert_se(fd > 0);
        close(fd);
        fd = open(file2, O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC, 0664);
        assert_se(fd > 0);
        close(fd);
        link1 = path_join(tmp_dir, "link1");
        assert_se(link1);
        assert_se(symlink("file1", link1) == 0);
        link2 = path_join(tmp_dir, "link2");
        assert_se(link1);
        assert_se(symlink("file2", link2) == 0);

        assert_se(path_is_mount_point(file1, NULL, AT_SYMLINK_FOLLOW) == 0);
        assert_se(path_is_mount_point(file1, NULL, 0) == 0);
        assert_se(path_is_mount_point(link1, NULL, AT_SYMLINK_FOLLOW) == 0);
        assert_se(path_is_mount_point(link1, NULL, 0) == 0);

        /* directory mountpoints */
        dir1 = path_join(tmp_dir, "dir1");
        assert_se(dir1);
        assert_se(mkdir(dir1, 0755) == 0);
        dirlink1 = path_join(tmp_dir, "dirlink1");
        assert_se(dirlink1);
        assert_se(symlink("dir1", dirlink1) == 0);
        dirlink1file = path_join(tmp_dir, "dirlink1file");
        assert_se(dirlink1file);
        assert_se(symlink("dirlink1/file", dirlink1file) == 0);
        dir2 = path_join(tmp_dir, "dir2");
        assert_se(dir2);
        assert_se(mkdir(dir2, 0755) == 0);

        assert_se(path_is_mount_point(dir1, NULL, AT_SYMLINK_FOLLOW) == 0);
        assert_se(path_is_mount_point(dir1, NULL, 0) == 0);
        assert_se(path_is_mount_point(dirlink1, NULL, AT_SYMLINK_FOLLOW) == 0);
        assert_se(path_is_mount_point(dirlink1, NULL, 0) == 0);

        /* file in subdirectory mountpoints */
        dir1file = path_join(dir1, "file");
        assert_se(dir1file);
        fd = open(dir1file, O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC, 0664);
        assert_se(fd > 0);
        close(fd);

        assert_se(path_is_mount_point(dir1file, NULL, AT_SYMLINK_FOLLOW) == 0);
        assert_se(path_is_mount_point(dir1file, NULL, 0) == 0);
        assert_se(path_is_mount_point(dirlink1file, NULL, AT_SYMLINK_FOLLOW) == 0);
        assert_se(path_is_mount_point(dirlink1file, NULL, 0) == 0);

        /* these tests will only work as root */
        if (mount(file1, file2, NULL, MS_BIND, NULL) >= 0) {
                int rf, rt, rdf, rdt, rlf, rlt, rl1f, rl1t;
                const char *file2d;

                /* files */
                /* capture results in vars, to avoid dangling mounts on failure */
                log_info("%s: %s", __func__, file2);
                rf = path_is_mount_point(file2, NULL, 0);
                rt = path_is_mount_point(file2, NULL, AT_SYMLINK_FOLLOW);

                file2d = strjoina(file2, "/");
                log_info("%s: %s", __func__, file2d);
                rdf = path_is_mount_point(file2d, NULL, 0);
                rdt = path_is_mount_point(file2d, NULL, AT_SYMLINK_FOLLOW);

                log_info("%s: %s", __func__, link2);
                rlf = path_is_mount_point(link2, NULL, 0);
                rlt = path_is_mount_point(link2, NULL, AT_SYMLINK_FOLLOW);

                assert_se(umount(file2) == 0);

                assert_se(rf == 1);
                assert_se(rt == 1);
                assert_se(rdf == -ENOTDIR);
                assert_se(rdt == -ENOTDIR);
                assert_se(rlf == 0);
                assert_se(rlt == 1);

                /* dirs */
                dir2file = path_join(dir2, "file");
                assert_se(dir2file);
                fd = open(dir2file, O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC, 0664);
                assert_se(fd > 0);
                close(fd);

                assert_se(mount(dir2, dir1, NULL, MS_BIND, NULL) >= 0);

                log_info("%s: %s", __func__, dir1);
                rf = path_is_mount_point(dir1, NULL, 0);
                rt = path_is_mount_point(dir1, NULL, AT_SYMLINK_FOLLOW);
                log_info("%s: %s", __func__, dirlink1);
                rlf = path_is_mount_point(dirlink1, NULL, 0);
                rlt = path_is_mount_point(dirlink1, NULL, AT_SYMLINK_FOLLOW);
                log_info("%s: %s", __func__, dirlink1file);
                /* its parent is a mount point, but not /file itself */
                rl1f = path_is_mount_point(dirlink1file, NULL, 0);
                rl1t = path_is_mount_point(dirlink1file, NULL, AT_SYMLINK_FOLLOW);

                assert_se(umount(dir1) == 0);

                assert_se(rf == 1);
                assert_se(rt == 1);
                assert_se(rlf == 0);
                assert_se(rlt == 1);
                assert_se(rl1f == 0);
                assert_se(rl1t == 0);

        } else
                printf("Skipping bind mount file test: %m\n");

        assert_se(rm_rf(tmp_dir, REMOVE_ROOT|REMOVE_PHYSICAL) == 0);
}

static void test_fd_is_mount_point(void) {
        _cleanup_close_ int fd = -1;

        log_info("/* %s */", __func__);

        fd = open("/", O_RDONLY|O_CLOEXEC|O_DIRECTORY|O_NOCTTY);
        assert_se(fd >= 0);

        /* Not allowed, since "/" is a path, not a plain filename */
        assert_se(fd_is_mount_point(fd, "/", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, ".", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, "./", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, "..", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, "../", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, "", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, "/proc", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, "/proc/", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, "proc/sys", 0) == -EINVAL);
        assert_se(fd_is_mount_point(fd, "proc/sys/", 0) == -EINVAL);

        /* This one definitely is a mount point */
        assert_se(fd_is_mount_point(fd, "proc", 0) > 0);
        assert_se(fd_is_mount_point(fd, "proc/", 0) > 0);

        /* /root's entire reason for being is to be on the root file system (i.e. not in /home/ which
         * might be split off), so that the user can always log in, so it cannot be a mount point unless
         * the system is borked. Let's allow for it to be missing though. */
        assert_se(IN_SET(fd_is_mount_point(fd, "root", 0), -ENOENT, 0));
        assert_se(IN_SET(fd_is_mount_point(fd, "root/", 0), -ENOENT, 0));
}

static void test_make_mount_point_inode(void) {
        _cleanup_(rm_rf_physical_and_freep) char *d = NULL;
        const char *src_file, *src_dir, *dst_file, *dst_dir;
        struct stat st;

        log_info("/* %s */", __func__);

        assert_se(mkdtemp_malloc(NULL, &d) >= 0);

        src_file = strjoina(d, "/src/file");
        src_dir = strjoina(d, "/src/dir");
        dst_file = strjoina(d, "/dst/file");
        dst_dir = strjoina(d, "/dst/dir");

        assert_se(mkdir_p(src_dir, 0755) >= 0);
        assert_se(mkdir_parents(dst_file, 0755) >= 0);
        assert_se(touch(src_file) >= 0);

        assert_se(make_mount_point_inode_from_path(src_file, dst_file, 0755) >= 0);
        assert_se(make_mount_point_inode_from_path(src_dir, dst_dir, 0755) >= 0);

        assert_se(stat(dst_dir, &st) == 0);
        assert_se(S_ISDIR(st.st_mode));
        assert_se(stat(dst_file, &st) == 0);
        assert_se(S_ISREG(st.st_mode));
        assert_se(!(S_IXUSR & st.st_mode));
        assert_se(!(S_IXGRP & st.st_mode));
        assert_se(!(S_IXOTH & st.st_mode));

        assert_se(unlink(dst_file) == 0);
        assert_se(rmdir(dst_dir) == 0);

        assert_se(stat(src_file, &st) == 0);
        assert_se(make_mount_point_inode_from_stat(&st, dst_file, 0755) >= 0);
        assert_se(stat(src_dir, &st) == 0);
        assert_se(make_mount_point_inode_from_stat(&st, dst_dir, 0755) >= 0);

        assert_se(stat(dst_dir, &st) == 0);
        assert_se(S_ISDIR(st.st_mode));
        assert_se(stat(dst_file, &st) == 0);
        assert_se(S_ISREG(st.st_mode));
        assert_se(!(S_IXUSR & st.st_mode));
        assert_se(!(S_IXGRP & st.st_mode));
        assert_se(!(S_IXOTH & st.st_mode));
}

int main(int argc, char *argv[]) {
        test_setup_logging(LOG_DEBUG);

#if 0 /// UNNEEDED by elogind
        /* let's move into our own mount namespace with all propagation from the host turned off, so that
         * /proc/self/mountinfo is static and constant for the whole time our test runs. */
        if (unshare(CLONE_NEWNS) < 0) {
                if (!ERRNO_IS_PRIVILEGE(errno))
                        return log_error_errno(errno, "Failed to detach mount namespace: %m");

                log_notice("Lacking privilege to create separate mount namespace, proceeding in originating mount namespace.");
        } else
                assert_se(mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL) >= 0);

        test_mount_propagation_flags("shared", 0, MS_SHARED);
        test_mount_propagation_flags("slave", 0, MS_SLAVE);
        test_mount_propagation_flags("private", 0, MS_PRIVATE);
        test_mount_propagation_flags(NULL, 0, 0);
        test_mount_propagation_flags("", 0, 0);
        test_mount_propagation_flags("xxxx", -EINVAL, 0);
        test_mount_propagation_flags(" ", -EINVAL, 0);
#endif // 0

        test_mnt_id();
        test_path_is_mount_point();
        test_fd_is_mount_point();
        test_make_mount_point_inode();

        return 0;
}
