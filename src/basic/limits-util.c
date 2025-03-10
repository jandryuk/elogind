/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "alloc-util.h"
#include "cgroup-util.h"
#include "limits-util.h"
#include "memory-util.h"
#include "parse-util.h"
#include "process-util.h"
#include "procfs-util.h"
#include "string-util.h"

/// Additional includes needed by elogind
#include <unistd.h>

uint64_t physical_memory(void) {
        _cleanup_free_ char *root = NULL, *value = NULL;
        uint64_t mem, lim;
        size_t ps;
        long sc;
        int r;

        /* We return this as uint64_t in case we are running as 32bit process on a 64bit kernel with huge amounts of
         * memory.
         *
         * In order to support containers nicely that have a configured memory limit we'll take the minimum of the
         * physically reported amount of memory and the limit configured for the root cgroup, if there is any. */

        sc = sysconf(_SC_PHYS_PAGES);
        assert(sc > 0);

        ps = page_size();
        mem = (uint64_t) sc * (uint64_t) ps;

        r = cg_get_root_path(&root);
        if (r < 0) {
                log_debug_errno(r, "Failed to determine root cgroup, ignoring cgroup memory limit: %m");
                return mem;
        }

        r = cg_all_unified();
        if (r < 0) {
                log_debug_errno(r, "Failed to determine root unified mode, ignoring cgroup memory limit: %m");
                return mem;
        }
        if (r > 0) {
                r = cg_get_attribute("memory", root, "memory.max", &value);
                if (r == -ENOENT) /* Field does not exist on the system's top-level cgroup, hence don't
                                   * complain. (Note that it might exist on our own root though, if we live
                                   * in a cgroup namespace, hence check anyway instead of not even
                                   * trying.) */
                        return mem;
                if (r < 0) {
                        log_debug_errno(r, "Failed to read memory.max cgroup attribute, ignoring cgroup memory limit: %m");
                        return mem;
                }

                if (streq(value, "max"))
                        return mem;
        } else {
                r = cg_get_attribute("memory", root, "memory.limit_in_bytes", &value);
                if (r < 0) {
                        log_debug_errno(r, "Failed to read memory.limit_in_bytes cgroup attribute, ignoring cgroup memory limit: %m");
                        return mem;
                }
        }

        r = safe_atou64(value, &lim);
        if (r < 0) {
                log_debug_errno(r, "Failed to parse cgroup memory limit '%s', ignoring: %m", value);
                return mem;
        }
        if (lim == UINT64_MAX)
                return mem;

        /* Make sure the limit is a multiple of our own page size */
        lim /= ps;
        lim *= ps;

        return MIN(mem, lim);
}

uint64_t physical_memory_scale(uint64_t v, uint64_t max) {
        uint64_t p, m, ps;

        /* Shortcut two special cases */
        if (v == 0)
                return 0;
        if (v == max)
                return physical_memory();

        assert(max > 0);

        /* Returns the physical memory size, multiplied by v divided by max. Returns UINT64_MAX on overflow. On success
         * the result is a multiple of the page size (rounds down). */

        ps = page_size();
        assert(ps > 0);

        p = physical_memory() / ps;
        assert(p > 0);

        if (v > UINT64_MAX / p)
                return UINT64_MAX;

        m = p * v;
        m /= max;

        if (m > UINT64_MAX / ps)
                return UINT64_MAX;

        return m * ps;
}

#if 0 /// UNNEEDED by elogind
uint64_t system_tasks_max(void) {
        uint64_t a = TASKS_MAX, b = TASKS_MAX;
        _cleanup_free_ char *root = NULL;
        int r;

        /* Determine the maximum number of tasks that may run on this system. We check three sources to determine this
         * limit:
         *
         * a) the maximum tasks value the kernel allows on this architecture
         * b) the cgroups pids_max attribute for the system
         * c) the kernel's configured maximum PID value
         *
         * And then pick the smallest of the three */

        r = procfs_tasks_get_limit(&a);
        if (r < 0)
                log_debug_errno(r, "Failed to read maximum number of tasks from /proc, ignoring: %m");

        r = cg_get_root_path(&root);
        if (r < 0)
                log_debug_errno(r, "Failed to determine cgroup root path, ignoring: %m");
        else {
                r = cg_get_attribute_as_uint64("pids", root, "pids.max", &b);
                if (r < 0)
                        log_debug_errno(r, "Failed to read pids.max attribute of cgroup root, ignoring: %m");
        }

        return MIN3(TASKS_MAX,
                    a <= 0 ? TASKS_MAX : a,
                    b <= 0 ? TASKS_MAX : b);
}

uint64_t system_tasks_max_scale(uint64_t v, uint64_t max) {
        uint64_t t, m;

        /* Shortcut two special cases */
        if (v == 0)
                return 0;
        if (v == max)
                return system_tasks_max();

        assert(max > 0);

        /* Multiply the system's task value by the fraction v/max. Hence, if max==100 this calculates percentages
         * relative to the system's maximum number of tasks. Returns UINT64_MAX on overflow. */

        t = system_tasks_max();
        assert(t > 0);

        if (v > UINT64_MAX / t) /* overflow? */
                return UINT64_MAX;

        m = t * v;
        return m / max;
}
#endif // 0
