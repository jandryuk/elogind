/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "sd-daemon.h"
#include "sd-device.h"

#include "alloc-util.h"
#include "bus-error.h"
#include "bus-locator.h"
#include "bus-log-control-api.h"
#include "bus-polkit.h"
#include "cgroup-util.h"
#include "daemon-util.h"
#include "def.h"
#include "device-util.h"
#include "dirent-util.h"
#include "fd-util.h"
#include "format-util.h"
#include "fs-util.h"
#include "logind-dbus.h"
#include "logind-seat-dbus.h"
#include "logind-session-dbus.h"
#include "logind-user-dbus.h"
#include "logind.h"
#include "main-func.h"
#include "parse-util.h"
#include "process-util.h"
#include "selinux-util.h"
#include "service-util.h"
#include "signal-util.h"
#include "strv.h"
#include "terminal-util.h"
/// Additional includes needed by elogind
#include "cgroup.h"       // From src/core/
#include "label.h"
#include "musl_missing.h"
#include "udev-util.h"
#include "user-util.h"

static Manager* manager_unref(Manager *m);
DEFINE_TRIVIAL_CLEANUP_FUNC(Manager*, manager_unref);

static int manager_new(Manager **ret) {
        _cleanup_(manager_unrefp) Manager *m = NULL;
        int r;

        assert(ret);

        m = new(Manager, 1);
        if (!m)
                return -ENOMEM;

        *m = (Manager) {
                .console_active_fd = -1,
#if 0 /// elogind does not support autospawning of vts
                .reserve_vt_fd = -1,
                .idle_action_not_before_usec = now(CLOCK_MONOTONIC),
#endif // 0
        };

        m->devices = hashmap_new(&string_hash_ops);
        m->seats = hashmap_new(&string_hash_ops);
        m->sessions = hashmap_new(&string_hash_ops);
        m->sessions_by_leader = hashmap_new(NULL);
        m->users = hashmap_new(NULL);
        m->inhibitors = hashmap_new(&string_hash_ops);
        m->buttons = hashmap_new(&string_hash_ops);

#if 0 /// elogind does not support units
        m->user_units = hashmap_new(&string_hash_ops);
        m->session_units = hashmap_new(&string_hash_ops);

        if (!m->devices || !m->seats || !m->sessions || !m->sessions_by_leader || !m->users || !m->inhibitors || !m->buttons || !m->user_units || !m->session_units)
#else // 0
        if (!m->devices || !m->seats || !m->sessions || !m->sessions_by_leader || !m->users || !m->inhibitors || !m->buttons)
#endif // 0
                return -ENOMEM;

#if 1 /// elogind needs some more data
        r = elogind_manager_new(m);
        if (r < 0) {
                return r;
        }
#endif // 1
        r = sd_event_default(&m->event);
        if (r < 0)
                return r;

#if 0 /// elogind uses its own signal handler, installed at elogind_manager_startup()
        r = sd_event_add_signal(m->event, NULL, SIGINT, NULL, NULL);
        if (r < 0)
                return r;

        r = sd_event_add_signal(m->event, NULL, SIGTERM, NULL, NULL);
        if (r < 0)
                return r;
#endif // 0

        (void) sd_event_set_watchdog(m->event, true);

        manager_reset_config(m);

        *ret = TAKE_PTR(m);
        return 0;
}

static Manager* manager_unref(Manager *m) {
        Session *session;
        User *u;
        Device *d;
        Seat *s;
        Inhibitor *i;
        Button *b;

        if (!m)
                return NULL;

        log_debug_elogind("%s", "Tearing down all references (manager_unref) ...");
        while ((session = hashmap_first(m->sessions)))
                session_free(session);

        while ((u = hashmap_first(m->users)))
                user_free(u);

        while ((d = hashmap_first(m->devices)))
                device_free(d);

        while ((s = hashmap_first(m->seats)))
                seat_free(s);

        while ((i = hashmap_first(m->inhibitors)))
                inhibitor_free(i);

        while ((b = hashmap_first(m->buttons)))
                button_free(b);

        hashmap_free(m->devices);
        hashmap_free(m->seats);
        hashmap_free(m->sessions);
        hashmap_free(m->sessions_by_leader);
        hashmap_free(m->users);
        hashmap_free(m->inhibitors);
        hashmap_free(m->buttons);
        hashmap_free(m->brightness_writers);

#if 0 /// elogind does not support systemd units.
        hashmap_free(m->user_units);
        hashmap_free(m->session_units);
#endif // 0

        sd_event_source_unref(m->idle_action_event_source);
        sd_event_source_unref(m->inhibit_timeout_source);
        sd_event_source_unref(m->scheduled_shutdown_timeout_source);
        sd_event_source_unref(m->nologin_timeout_source);
        sd_event_source_unref(m->wall_message_timeout_source);

        sd_event_source_unref(m->console_active_event_source);
        sd_event_source_unref(m->lid_switch_ignore_event_source);

#if ENABLE_UTMP
        sd_event_source_unref(m->utmp_event_source);
#endif

        safe_close(m->console_active_fd);

        sd_device_monitor_unref(m->device_seat_monitor);
        sd_device_monitor_unref(m->device_monitor);
        sd_device_monitor_unref(m->device_vcsa_monitor);
        sd_device_monitor_unref(m->device_button_monitor);

        if (m->unlink_nologin)
                (void) unlink_or_warn("/run/nologin");

        bus_verify_polkit_async_registry_free(m->polkit_registry);

        sd_bus_flush_close_unref(m->bus);
        sd_event_unref(m->event);

#if 0 /// elogind does not support autospawning of vts
        safe_close(m->reserve_vt_fd);
#endif // 0
#if 1 /// elogind has to free its own data
        elogind_manager_free(m);
#endif // 1

        strv_free(m->kill_only_users);
        strv_free(m->kill_exclude_users);

        free(m->scheduled_shutdown_type);
        free(m->scheduled_shutdown_tty);
        free(m->wall_message);
#if 0 /// UNNEEDED by elogind
        free(m->action_job);
#endif // 0

        strv_free(m->efi_boot_loader_entries);
        free(m->efi_loader_entry_one_shot);

        return mfree(m);
}

static int manager_enumerate_devices(Manager *m) {
        _cleanup_(sd_device_enumerator_unrefp) sd_device_enumerator *e = NULL;
        sd_device *d;
        int r;

        assert(m);

        /* Loads devices from udev and creates seats for them as
         * necessary */

        r = sd_device_enumerator_new(&e);
        if (r < 0)
                return r;

        r = sd_device_enumerator_add_match_tag(e, "master-of-seat");
        if (r < 0)
                return r;

        FOREACH_DEVICE(e, d) {
                int k;

                k = manager_process_seat_device(m, d);
                if (k < 0)
                        r = k;
        }

        return r;
}

static int manager_enumerate_buttons(Manager *m) {
        _cleanup_(sd_device_enumerator_unrefp) sd_device_enumerator *e = NULL;
        sd_device *d;
        int r;

        assert(m);

        /* Loads buttons from udev */

        if (manager_all_buttons_ignored(m))
                return 0;

        r = sd_device_enumerator_new(&e);
        if (r < 0)
                return r;

        r = sd_device_enumerator_add_match_subsystem(e, "input", true);
        if (r < 0)
                return r;

        r = sd_device_enumerator_add_match_tag(e, "power-switch");
        if (r < 0)
                return r;

        FOREACH_DEVICE(e, d) {
                int k;

                k = manager_process_button_device(m, d);
                if (k < 0)
                        r = k;
        }

        return r;
}

static int manager_enumerate_seats(Manager *m) {
        _cleanup_closedir_ DIR *d = NULL;
        struct dirent *de;
        int r = 0;

        assert(m);

        /* This loads data about seats stored on disk, but does not
         * actually create any seats. Removes data of seats that no
         * longer exist. */

        d = opendir("/run/systemd/seats");
        if (!d) {
                if (errno == ENOENT)
                        return 0;

                return log_error_errno(errno, "Failed to open /run/systemd/seats: %m");
        }

        FOREACH_DIRENT(de, d, return -errno) {
                Seat *s;
                int k;

                if (!dirent_is_file(de))
                        continue;

                s = hashmap_get(m->seats, de->d_name);
                if (!s) {
                        log_debug_elogind("Removing stale seat at /run/systemd/seats/%s", de->d_name);
                        if (unlinkat(dirfd(d), de->d_name, 0) < 0)
                                log_warning("Failed to remove /run/systemd/seats/%s: %m",
                                            de->d_name);
                        continue;
                }

                log_debug_elogind("Loading seat /run/systemd/seats/%s", de->d_name);
                k = seat_load(s);
                if (k < 0)
                        r = k;
        }

        return r;
}

static int manager_enumerate_linger_users(Manager *m) {
        _cleanup_closedir_ DIR *d = NULL;
        struct dirent *de;
        int r = 0;

        assert(m);

        d = opendir("/var/lib/elogind/linger");
        if (!d) {
                if (errno == ENOENT)
                        return 0;

                return log_error_errno(errno, "Failed to open /var/lib/elogind/linger/: %m");
        }

        FOREACH_DIRENT(de, d, return -errno) {
                int k;

                dirent_ensure_type(d, de);
                if (!dirent_is_file(de))
                        continue;

                k = manager_add_user_by_name(m, de->d_name, NULL);
                if (k < 0)
                        r = log_warning_errno(k, "Couldn't add lingering user %s, ignoring: %m", de->d_name);
        }

        return r;
}

static int manager_enumerate_users(Manager *m) {
        _cleanup_closedir_ DIR *d = NULL;
        struct dirent *de;
        int r, k;

        assert(m);

        /* Add lingering users */
        r = manager_enumerate_linger_users(m);

        /* Read in user data stored on disk */
        d = opendir("/run/systemd/users");
        if (!d) {
                if (errno == ENOENT)
                        return 0;

                return log_error_errno(errno, "Failed to open /run/systemd/users: %m");
        }

        FOREACH_DIRENT(de, d, return -errno) {
                User *u;
                uid_t uid;

                if (!dirent_is_file(de))
                        continue;

                k = parse_uid(de->d_name, &uid);
                if (k < 0) {
                        r = log_warning_errno(k, "Failed to parse filename /run/systemd/users/%s as UID.", de->d_name);
                        continue;
                }

                k = manager_add_user_by_uid(m, uid, &u);
                if (k < 0) {
                        r = log_warning_errno(k, "Failed to add user by file name %s, ignoring: %m", de->d_name);
                        continue;
                }

                log_debug_elogind("Loading user %d \"%s\"", u->user_record->uid, strna(u->user_record->user_name));
                user_add_to_gc_queue(u);

                k = user_load(u);
                if (k < 0)
                        r = k;
        }

        return r;
}

static int parse_fdname(const char *fdname, char **session_id, dev_t *dev) {
        _cleanup_strv_free_ char **parts = NULL;
        _cleanup_free_ char *id = NULL;
        unsigned major, minor;
        int r;

        parts = strv_split(fdname, "-");
        if (!parts)
                return -ENOMEM;
        if (strv_length(parts) != 5)
                return -EINVAL;

        if (!streq(parts[0], "session"))
                return -EINVAL;

        id = strdup(parts[1]);
        if (!id)
                return -ENOMEM;

        if (!streq(parts[2], "device"))
                return -EINVAL;

        r = safe_atou(parts[3], &major);
        if (r < 0)
                return r;
        r = safe_atou(parts[4], &minor);
        if (r < 0)
                return r;

        *dev = makedev(major, minor);
        *session_id = TAKE_PTR(id);

        return 0;
}

static int deliver_fd(Manager *m, const char *fdname, int fd) {
        _cleanup_free_ char *id = NULL;
        SessionDevice *sd;
        struct stat st;
        Session *s;
        dev_t dev;
        int r;

        assert(m);
        assert(fd >= 0);

        r = parse_fdname(fdname, &id, &dev);
        if (r < 0)
                return log_debug_errno(r, "Failed to parse fd name %s: %m", fdname);

        s = hashmap_get(m->sessions, id);
        if (!s)
                /* If the session doesn't exist anymore, the associated session device attached to this fd
                 * doesn't either. Let's simply close this fd. */
                return log_debug_errno(SYNTHETIC_ERRNO(ENXIO), "Failed to attach fd for unknown session: %s", id);

        if (fstat(fd, &st) < 0)
                /* The device is allowed to go away at a random point, in which case fstat() failing is
                 * expected. */
                return log_debug_errno(errno, "Failed to stat device fd for session %s: %m", id);

        if (!S_ISCHR(st.st_mode) || st.st_rdev != dev)
                return log_debug_errno(SYNTHETIC_ERRNO(ENODEV), "Device fd doesn't point to the expected character device node");

        sd = hashmap_get(s->devices, &dev);
        if (!sd)
                /* Weird, we got an fd for a session device which wasn't recorded in the session state
                 * file... */
                return log_warning_errno(SYNTHETIC_ERRNO(ENODEV), "Got fd for missing session device [%u:%u] in session %s",
                                         major(dev), minor(dev), s->id);

        log_debug("Attaching fd to session device [%u:%u] for session %s",
                  major(dev), minor(dev), s->id);

        session_device_attach_fd(sd, fd, s->was_active);
        return 0;
}

static int manager_attach_fds(Manager *m) {
        _cleanup_strv_free_ char **fdnames = NULL;
        int n;

        /* Upon restart, PID1 will send us back all fds of session devices that we previously opened. Each
         * file descriptor is associated with a given session. The session ids are passed through FDNAMES. */

        n = sd_listen_fds_with_names(true, &fdnames);
        if (n < 0)
                return log_warning_errno(n, "Failed to acquire passed fd list: %m");
        if (n == 0)
                return 0;

        for (int i = 0; i < n; i++) {
                int fd = SD_LISTEN_FDS_START + i;

                if (deliver_fd(m, fdnames[i], fd) >= 0)
                        continue;

                /* Hmm, we couldn't deliver the fd to any session device object? If so, let's close the fd */
                safe_close(fd);

                /* Remove from fdstore as well */
                (void) sd_notifyf(false,
                                  "FDSTOREREMOVE=1\n"
                                  "FDNAME=%s", fdnames[i]);
        }

        return 0;
}

static int manager_enumerate_sessions(Manager *m) {
        _cleanup_closedir_ DIR *d = NULL;
        struct dirent *de;
        int r = 0, k;

        assert(m);

        /* Read in session data stored on disk */
        d = opendir("/run/systemd/sessions");
        if (!d) {
                if (errno == ENOENT)
                        return 0;

                return log_error_errno(errno, "Failed to open /run/systemd/sessions: %m");
        }

        FOREACH_DIRENT(de, d, return -errno) {
                struct Session *s;

                if (!dirent_is_file(de))
                        continue;

                log_debug_elogind("Adding session /run/systemd/sessions/%s", de->d_name);
                k = manager_add_session(m, de->d_name, &s);
                if (k < 0) {
                        r = log_warning_errno(k, "Failed to add session by file name %s, ignoring: %m", de->d_name);
                        continue;
                }

                session_add_to_gc_queue(s);

                log_debug_elogind("Loading session /run/systemd/sessions/%s", de->d_name);
                k = session_load(s);
                if (k < 0)
                        r = k;
        }

        /* We might be restarted and PID1 could have sent us back the session device fds we previously
         * saved. */
        (void) manager_attach_fds(m);

        return r;
}

static int manager_enumerate_inhibitors(Manager *m) {
        _cleanup_closedir_ DIR *d = NULL;
        struct dirent *de;
        int r = 0;

        assert(m);

        d = opendir("/run/systemd/inhibit");
        if (!d) {
                if (errno == ENOENT)
                        return 0;

                return log_error_errno(errno, "Failed to open /run/systemd/inhibit: %m");
        }

        FOREACH_DIRENT(de, d, return -errno) {
                int k;
                Inhibitor *i;

                if (!dirent_is_file(de))
                        continue;

                k = manager_add_inhibitor(m, de->d_name, &i);
                if (k < 0) {
                        r = log_warning_errno(k, "Couldn't add inhibitor %s, ignoring: %m", de->d_name);
                        continue;
                }

                log_debug_elogind("Loading inhibitor /run/systemd/inhibit/%s", de->d_name);
                k = inhibitor_load(i);
                if (k < 0)
                        r = k;
        }

        return r;
}

static int manager_dispatch_seat_udev(sd_device_monitor *monitor, sd_device *device, void *userdata) {
        Manager *m = userdata;

        assert(m);
        assert(device);

        manager_process_seat_device(m, device);
        return 0;
}

static int manager_dispatch_device_udev(sd_device_monitor *monitor, sd_device *device, void *userdata) {
        Manager *m = userdata;

        assert(m);
        assert(device);

        manager_process_seat_device(m, device);
        return 0;
}

#if 0 /// UNNEEDED by elogind
static int manager_dispatch_vcsa_udev(sd_device_monitor *monitor, sd_device *device, void *userdata) {
        Manager *m = userdata;
        const char *name;

        assert(m);
        assert(device);

        /* Whenever a VCSA device is removed try to reallocate our
         * VTs, to make sure our auto VTs never go away. */

        if (sd_device_get_sysname(device, &name) >= 0 &&
            startswith(name, "vcsa") &&
            device_for_action(device, SD_DEVICE_REMOVE))
                seat_preallocate_vts(m->seat0);

        return 0;
}
#endif // 0

static int manager_dispatch_button_udev(sd_device_monitor *monitor, sd_device *device, void *userdata) {
        Manager *m = userdata;

        assert(m);
        assert(device);

        manager_process_button_device(m, device);
        return 0;
}

static int manager_dispatch_console(sd_event_source *s, int fd, uint32_t revents, void *userdata) {
        Manager *m = userdata;

        assert(m);
        assert(m->seat0);
        assert(m->console_active_fd == fd);

        seat_read_active_vt(m->seat0);
        return 0;
}

#if 0 /// UNNEEDED by elogind
static int manager_reserve_vt(Manager *m) {
        _cleanup_free_ char *p = NULL;

        assert(m);

        if (m->reserve_vt <= 0)
                return 0;

        if (asprintf(&p, "/dev/tty%u", m->reserve_vt) < 0)
                return log_oom();

        m->reserve_vt_fd = open(p, O_RDWR|O_NOCTTY|O_CLOEXEC|O_NONBLOCK);
        if (m->reserve_vt_fd < 0) {

                /* Don't complain on VT-less systems */
                if (errno != ENOENT)
                        log_warning_errno(errno, "Failed to pin reserved VT: %m");
                return -errno;
        }

        return 0;
}
#endif // 0

static int manager_connect_bus(Manager *m) {
        int r;

        assert(m);
        assert(!m->bus);

        r = sd_bus_default_system(&m->bus);
        if (r < 0)
                return log_error_errno(r, "Failed to connect to system bus: %m");

        r = bus_add_implementation(m->bus, &manager_object, m);
        if (r < 0)
                return r;

        r = bus_log_control_api_register(m->bus);
        if (r < 0)
                return r;

#if 0 /// elogind does not support systemd as PID 1
        r = bus_match_signal_async(m->bus, NULL, bus_systemd_mgr, "JobRemoved", match_job_removed, NULL, m);
        if (r < 0)
                return log_error_errno(r, "Failed to request match for JobRemoved: %m");

        r = bus_match_signal_async(m->bus, NULL, bus_systemd_mgr, "UnitRemoved", match_unit_removed, NULL, m);
        if (r < 0)
                return log_error_errno(r, "Failed to request match for UnitRemoved: %m");

        r = sd_bus_match_signal_async(
                        m->bus,
                        NULL,
                        "org.freedesktop.systemd1",
                        NULL,
                        "org.freedesktop.DBus.Properties",
                        "PropertiesChanged",
                        match_properties_changed, NULL, m);
        if (r < 0)
                return log_error_errno(r, "Failed to request match for PropertiesChanged: %m");

        r = bus_match_signal_async(m->bus, NULL, bus_systemd_mgr, "Reloading", match_reloading, NULL, m);
        if (r < 0)
                return log_error_errno(r, "Failed to request match for Reloading: %m");

        r = bus_call_method_async(m->bus, NULL, bus_systemd_mgr, "Subscribe", NULL, NULL, NULL);
        if (r < 0)
                return log_error_errno(r, "Failed to enable subscription: %m");
#endif // 0

        r = sd_bus_request_name_async(m->bus, NULL, "org.freedesktop.login1", 0, NULL, NULL);
        if (r < 0)
                return log_error_errno(r, "Failed to request name: %m");

        r = sd_bus_attach_event(m->bus, m->event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0)
                return log_error_errno(r, "Failed to attach bus to event loop: %m");

#if 0 /// elogind has to setup its release agent
        return 0;
#else // 0
        r = elogind_setup_cgroups_agent(m);

        return r;
#endif // 0
}

static int manager_vt_switch(sd_event_source *src, const struct signalfd_siginfo *si, void *data) {
        Manager *m = data;
        Session *active, *iter;

        /*
         * We got a VT-switch signal and we have to acknowledge it immediately.
         * Preferably, we'd just use m->seat0->active->vtfd, but unfortunately,
         * old user-space might run multiple sessions on a single VT, *sigh*.
         * Therefore, we have to iterate all sessions and find one with a vtfd
         * on the requested VT.
         * As only VTs with active controllers have VT_PROCESS set, our current
         * notion of the active VT might be wrong (for instance if the switch
         * happens while we setup VT_PROCESS). Therefore, read the current VT
         * first and then use s->active->vtnr as reference. Note that this is
         * not racy, as no further VT-switch can happen as long as we're in
         * synchronous VT_PROCESS mode.
         */

        assert(m->seat0);
        seat_read_active_vt(m->seat0);

        active = m->seat0->active;
        if (!active || active->vtnr < 1) {
                _cleanup_close_ int fd = -1;
                int r;

                /* We are requested to acknowledge the VT-switch signal by the kernel but
                 * there's no registered sessions for the current VT. Normally this
                 * shouldn't happen but something wrong might have happened when we tried
                 * to release the VT. Better be safe than sorry, and try to release the VT
                 * one more time otherwise the user will be locked with the current VT. */

                log_warning("Received VT_PROCESS signal without a registered session, restoring VT.");

                /* At this point we only have the kernel mapping for referring to the
                 * current VT. */
                fd = open_terminal("/dev/tty0", O_RDWR|O_NOCTTY|O_CLOEXEC|O_NONBLOCK);
                if (fd < 0) {
                        log_warning_errno(fd, "Failed to open, ignoring: %m");
                        return 0;
                }

                r = vt_release(fd, true);
                if (r < 0)
                        log_warning_errno(r, "Failed to release VT, ignoring: %m");

                return 0;
        }

        if (active->vtfd >= 0) {
                session_leave_vt(active);
        } else {
                LIST_FOREACH(sessions_by_seat, iter, m->seat0->sessions) {
                        if (iter->vtnr == active->vtnr && iter->vtfd >= 0) {
                                session_leave_vt(iter);
                                break;
                        }
                }
        }

        return 0;
}

static int manager_connect_console(Manager *m) {
        int r;

        assert(m);
        assert(m->console_active_fd < 0);

        /* On certain systems (such as S390, Xen, and containers) /dev/tty0 does not exist (as there is no VC), so
         * don't fail if we can't open it. */

        if (access("/dev/tty0", F_OK) < 0)
                return 0;

        m->console_active_fd = open("/sys/class/tty/tty0/active", O_RDONLY|O_NOCTTY|O_CLOEXEC);
        if (m->console_active_fd < 0) {

                /* On some systems /dev/tty0 may exist even though /sys/class/tty/tty0 does not. These are broken, but
                 * common. Let's complain but continue anyway. */
                if (errno == ENOENT) {
                        log_warning_errno(errno, "System has /dev/tty0 but not /sys/class/tty/tty0/active which is broken, ignoring: %m");
                        return 0;
                }

                return log_error_errno(errno, "Failed to open /sys/class/tty/tty0/active: %m");
        }

        r = sd_event_add_io(m->event, &m->console_active_event_source, m->console_active_fd, 0, manager_dispatch_console, m);
        if (r < 0)
                return log_error_errno(r, "Failed to watch foreground console: %m");

        /*
         * SIGRTMIN is used as global VT-release signal, SIGRTMIN + 1 is used
         * as VT-acquire signal. We ignore any acquire-events (yes, we still
         * have to provide a valid signal-number for it!) and acknowledge all
         * release events immediately.
         */

        if (SIGRTMIN + 1 > SIGRTMAX)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "Not enough real-time signals available: %u-%u",
                                       SIGRTMIN, SIGRTMAX);

        assert_se(ignore_signals(SIGRTMIN + 1) >= 0);
        assert_se(sigprocmask_many(SIG_BLOCK, NULL, SIGRTMIN, -1) >= 0);

        r = sd_event_add_signal(m->event, NULL, SIGRTMIN, manager_vt_switch, m);
        if (r < 0)
                return log_error_errno(r, "Failed to subscribe to signal: %m");

        return 0;
}

static int manager_connect_udev(Manager *m) {
        int r;

        assert(m);
        assert(!m->device_seat_monitor);
        assert(!m->device_monitor);
        assert(!m->device_vcsa_monitor);
        assert(!m->device_button_monitor);

        r = sd_device_monitor_new(&m->device_seat_monitor);
        if (r < 0)
                return r;

        r = sd_device_monitor_filter_add_match_tag(m->device_seat_monitor, "master-of-seat");
        if (r < 0)
                return r;

        r = sd_device_monitor_attach_event(m->device_seat_monitor, m->event);
        if (r < 0)
                return r;

        r = sd_device_monitor_start(m->device_seat_monitor, manager_dispatch_seat_udev, m);
        if (r < 0)
                return r;

        (void) sd_event_source_set_description(sd_device_monitor_get_event_source(m->device_seat_monitor), "logind-seat-monitor");

        r = sd_device_monitor_new(&m->device_monitor);
        if (r < 0)
                return r;

        r = sd_device_monitor_filter_add_match_subsystem_devtype(m->device_monitor, "input", NULL);
        if (r < 0)
                return r;

        r = sd_device_monitor_filter_add_match_subsystem_devtype(m->device_monitor, "graphics", NULL);
        if (r < 0)
                return r;

        r = sd_device_monitor_filter_add_match_subsystem_devtype(m->device_monitor, "drm", NULL);
        if (r < 0)
                return r;

        r = sd_device_monitor_attach_event(m->device_monitor, m->event);
        if (r < 0)
                return r;

        r = sd_device_monitor_start(m->device_monitor, manager_dispatch_device_udev, m);
        if (r < 0)
                return r;

        (void) sd_event_source_set_description(sd_device_monitor_get_event_source(m->device_monitor), "logind-device-monitor");

        /* Don't watch keys if nobody cares */
        if (!manager_all_buttons_ignored(m)) {
                r = sd_device_monitor_new(&m->device_button_monitor);
                if (r < 0)
                        return r;

                r = sd_device_monitor_filter_add_match_tag(m->device_button_monitor, "power-switch");
                if (r < 0)
                        return r;

                r = sd_device_monitor_filter_add_match_subsystem_devtype(m->device_button_monitor, "input", NULL);
                if (r < 0)
                        return r;

                r = sd_device_monitor_attach_event(m->device_button_monitor, m->event);
                if (r < 0)
                        return r;

                r = sd_device_monitor_start(m->device_button_monitor, manager_dispatch_button_udev, m);
                if (r < 0)
                        return r;

                (void) sd_event_source_set_description(sd_device_monitor_get_event_source(m->device_button_monitor), "logind-button-monitor");
        }

#if 0 /// elogind does not support autospawning of vts
        /* Don't bother watching VCSA devices, if nobody cares */
        if (m->n_autovts > 0 && m->console_active_fd >= 0) {

                r = sd_device_monitor_new(&m->device_vcsa_monitor);
                if (r < 0)
                        return r;

                r = sd_device_monitor_filter_add_match_subsystem_devtype(m->device_vcsa_monitor, "vc", NULL);
                if (r < 0)
                        return r;

                r = sd_device_monitor_attach_event(m->device_vcsa_monitor, m->event);
                if (r < 0)
                        return r;

                r = sd_device_monitor_start(m->device_vcsa_monitor, manager_dispatch_vcsa_udev, m);
                if (r < 0)
                        return r;

                (void) sd_event_source_set_description(sd_device_monitor_get_event_source(m->device_vcsa_monitor), "logind-vcsa-monitor");
        }
#endif // 0

        return 0;
}

static void manager_gc(Manager *m, bool drop_not_started) {
        Seat *seat;
        Session *session;
        User *user;

        assert(m);

        while ((seat = m->seat_gc_queue)) {
                LIST_REMOVE(gc_queue, m->seat_gc_queue, seat);
                seat->in_gc_queue = false;

                if (seat_may_gc(seat, drop_not_started)) {
                        seat_stop(seat, /* force = */ false);
                        seat_free(seat);
                }
        }

        while ((session = m->session_gc_queue)) {
                LIST_REMOVE(gc_queue, m->session_gc_queue, session);
                session->in_gc_queue = false;

                /* First, if we are not closing yet, initiate stopping. */
                if (session_may_gc(session, drop_not_started) &&
                    session_get_state(session) != SESSION_CLOSING)
                        (void) session_stop(session, /* force = */ false);

                /* Normally, this should make the session referenced again, if it doesn't then let's get rid
                 * of it immediately. */
                if (session_may_gc(session, drop_not_started)) {
                        (void) session_finalize(session);
                        session_free(session);
                }
        }

        while ((user = m->user_gc_queue)) {
                LIST_REMOVE(gc_queue, m->user_gc_queue, user);
                user->in_gc_queue = false;

                /* First step: queue stop jobs */
                if (user_may_gc(user, drop_not_started))
                        (void) user_stop(user, false);

                /* Second step: finalize user */
                if (user_may_gc(user, drop_not_started)) {
                        (void) user_finalize(user);
                        user_free(user);
                }
        }
}

static int manager_dispatch_idle_action(sd_event_source *s, uint64_t t, void *userdata) {
        Manager *m = userdata;
        struct dual_timestamp since;
        usec_t n, elapse;
        int r;

        assert(m);

        if (m->idle_action == HANDLE_IGNORE ||
            m->idle_action_usec <= 0)
                return 0;

        n = now(CLOCK_MONOTONIC);

        r = manager_get_idle_hint(m, &since);
        if (r <= 0)
                /* Not idle. Let's check if after a timeout it might be idle then. */
                elapse = n + m->idle_action_usec;
        else {
                /* Idle! Let's see if it's time to do something, or if
                 * we shall sleep for longer. */

                if (n >= since.monotonic + m->idle_action_usec &&
                    (m->idle_action_not_before_usec <= 0 || n >= m->idle_action_not_before_usec + m->idle_action_usec)) {
                        log_info("System idle. Doing %s operation.", handle_action_to_string(m->idle_action));

                        manager_handle_action(m, 0, m->idle_action, false, false);
                        m->idle_action_not_before_usec = n;
                }

                elapse = MAX(since.monotonic, m->idle_action_not_before_usec) + m->idle_action_usec;
        }

        if (!m->idle_action_event_source) {

                r = sd_event_add_time(
                                m->event,
                                &m->idle_action_event_source,
                                CLOCK_MONOTONIC,
                                elapse, USEC_PER_SEC*30,
                                manager_dispatch_idle_action, m);
                if (r < 0)
                        return log_error_errno(r, "Failed to add idle event source: %m");

                r = sd_event_source_set_priority(m->idle_action_event_source, SD_EVENT_PRIORITY_IDLE+10);
                if (r < 0)
                        return log_error_errno(r, "Failed to set idle event source priority: %m");
        } else {
                r = sd_event_source_set_time(m->idle_action_event_source, elapse);
                if (r < 0)
                        return log_error_errno(r, "Failed to set idle event timer: %m");

                r = sd_event_source_set_enabled(m->idle_action_event_source, SD_EVENT_ONESHOT);
                if (r < 0)
                        return log_error_errno(r, "Failed to enable idle event timer: %m");
        }

        return 0;
}

static int manager_dispatch_reload_signal(sd_event_source *s, const struct signalfd_siginfo *si, void *userdata) {
        Manager *m = userdata;
        int r;

        manager_reset_config(m);
        r = manager_parse_config_file(m);
        if (r < 0)
                log_warning_errno(r, "Failed to parse config file, using defaults: %m");
        else
                log_info("Config file reloaded.");

#if 1 /// elogind needs an Add-On for sleep configuration
        elogind_manager_reset_config(m);
#endif // 1
        return 0;
}

static int manager_startup(Manager *m) {
        int r;
        Seat *seat;
        Session *session;
        User *user;
        Button *button;
        Inhibitor *inhibitor;

        assert(m);

        r = sd_event_add_signal(m->event, NULL, SIGHUP, manager_dispatch_reload_signal, m);
        if (r < 0)
                return log_error_errno(r, "Failed to register SIGHUP handler: %m");

#if 1 /// install elogind specific signal handlers
        r = elogind_manager_startup(m);
        if (r < 0) {
                return log_error_errno(r, "Failed to register elogind signal handlers: %m");
        }
#endif // 1
        /* Connect to utmp */
        manager_connect_utmp(m);

        /* Connect to console */
        r = manager_connect_console(m);
        if (r < 0)
                return r;

        /* Connect to udev */
        r = manager_connect_udev(m);
        if (r < 0)
                return log_error_errno(r, "Failed to create udev watchers: %m");

        /* Connect to the bus */
        r = manager_connect_bus(m);
        if (r < 0)
                return r;

        /* Instantiate magic seat 0 */
        r = manager_add_seat(m, "seat0", &m->seat0);
        if (r < 0)
                return log_error_errno(r, "Failed to add seat0: %m");

        r = manager_set_lid_switch_ignore(m, 0 + m->holdoff_timeout_usec);
        if (r < 0)
                log_warning_errno(r, "Failed to set up lid switch ignore event source: %m");

        /* Deserialize state */
        r = manager_enumerate_devices(m);
        if (r < 0)
                log_warning_errno(r, "Device enumeration failed: %m");

        r = manager_enumerate_seats(m);
        if (r < 0)
                log_warning_errno(r, "Seat enumeration failed: %m");

        r = manager_enumerate_users(m);
        if (r < 0)
                log_warning_errno(r, "User enumeration failed: %m");

        r = manager_enumerate_sessions(m);
        if (r < 0)
                log_warning_errno(r, "Session enumeration failed: %m");

        r = manager_enumerate_inhibitors(m);
        if (r < 0)
                log_warning_errno(r, "Inhibitor enumeration failed: %m");

        r = manager_enumerate_buttons(m);
        if (r < 0)
                log_warning_errno(r, "Button enumeration failed: %m");

        /* Remove stale objects before we start them */
        manager_gc(m, false);

        /* Reserve the special reserved VT */
#if 0 /// elogind does not support autospawning of vts
        manager_reserve_vt(m);
#endif // 0

        /* Read in utmp if it exists */
        manager_read_utmp(m);

        /* And start everything */
        HASHMAP_FOREACH(seat, m->seats)
                (void) seat_start(seat);

        HASHMAP_FOREACH(user, m->users)
                (void) user_start(user);

        HASHMAP_FOREACH(session, m->sessions)
                (void) session_start(session, NULL, NULL);

        HASHMAP_FOREACH(inhibitor, m->inhibitors) {
                (void) inhibitor_start(inhibitor);

                /* Let's see if the inhibitor is dead now, then remove it */
                if (inhibitor_is_orphan(inhibitor)) {
                        inhibitor_stop(inhibitor);
                        inhibitor_free(inhibitor);
                }
        }

        HASHMAP_FOREACH(button, m->buttons)
                button_check_switches(button);

        manager_dispatch_idle_action(NULL, 0, m);

        return 0;
}

static int manager_run(Manager *m) {
        int r;

        assert(m);

        for (;;) {
                r = sd_event_get_state(m->event);
                if (r < 0)
                        return r;
                if (r == SD_EVENT_FINISHED)
                        return 0;

                manager_gc(m, true);

                r = manager_dispatch_delayed(m, false);
                if (r < 0)
                        return r;
                if (r > 0)
                        continue;

                r = sd_event_run(m->event, UINT64_MAX);
                if (r < 0)
                        return r;
        }
}

static int run(int argc, char *argv[]) {
        _cleanup_(manager_unrefp) Manager *m = NULL;
        _cleanup_(notify_on_cleanup) const char *notify_message = NULL;
        int r;

        elogind_set_program_name(argv[0]);
        log_set_facility(LOG_AUTH);
        log_setup();

#if 1 /// perform extra checks for elogind startup, and fork if wanted
        bool has_forked = false;
        r = elogind_startup(argc, argv, &has_forked);
        log_debug_elogind("elogind_startup() exited with %d", r);
        if (r) {
                return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
        }
        // If we forked, we are in the grandchild, the daemon, now.
#endif // 1

#if 0 /// This is elogind
        r = service_parse_argv("systemd-logind.service",
                               "Manager for user logins and devices and privileged operations.",
                               BUS_IMPLEMENTATIONS(&manager_object,
                                                   &log_control_object),
                               argc, argv);
        if (r <= 0)
                return r;
#else // 0
        if (!has_forked) {
                r = service_parse_argv("elogind",
                                       "Manager for user logins and devices and privileged operations.",
                                       BUS_IMPLEMENTATIONS(&manager_object, &log_control_object),
                                       argc, argv);
                log_debug_elogind("service_parse_argv() returned %d", r);
                if (r <= 0) {
                        return r;
                }
        }
#endif // 0

        umask(0022);

        r = mac_selinux_init();
        if (r < 0)
                return r;

#if 0 /// elogind can not rely on systemd to help, so we need a bit more effort than this
        /* Always create the directories people can create inotify watches in. Note that some applications
         * might check for the existence of /run/systemd/seats/ to determine whether logind is available, so
         * please always make sure these directories are created early on and unconditionally. */
        (void) mkdir_label("/run/systemd/seats", 0755);
        (void) mkdir_label("/run/systemd/users", 0755);
        (void) mkdir_label("/run/systemd/sessions", 0755);
#else // 0
        r = mkdir_label("/run/systemd", 0755);
        if ((r < 0) && (-EEXIST != r)) {
                return log_error_errno(r, "Failed to create /run/systemd : %m");
        }
        r = mkdir_label("/run/systemd/seats", 0755);
        if (r < 0 && (-EEXIST != r)) {
                return log_error_errno(r, "Failed to create /run/systemd/seats : %m");
        }
        r = mkdir_label("/run/systemd/users", 0755);
        if (r < 0 && (-EEXIST != r)) {
                return log_error_errno(r, "Failed to create /run/systemd/users : %m");
        }
        r = mkdir_label("/run/systemd/sessions", 0755);
        if (r < 0 && (-EEXIST != r)) {
                return log_error_errno(r, "Failed to create /run/systemd/sessions : %m");
        }
        r = mkdir_label("/run/systemd/machines", 0755);
        if (r < 0 && (-EEXIST != r)) {
                return log_error_errno(r, "Failed to create /run/systemd/machines : %m");
        }
#endif // 0

#if 0 /// elogind also blocks SIGQUIT, and installs a signal handler for it
        assert_se(sigprocmask_many(SIG_BLOCK, NULL, SIGHUP, SIGTERM, SIGINT, SIGCHLD, -1) >= 0);
#else // 0
        assert_se(sigprocmask_many(SIG_BLOCK, NULL, SIGHUP, SIGTERM, SIGINT, SIGCHLD, SIGQUIT, -1) >= 0);
#endif // 0

        log_debug_elogind("%s", "Creating manager...");
        r = manager_new(&m);
        if (r < 0)
                return log_error_errno(r, "Failed to allocate manager object: %m");

        (void) manager_parse_config_file(m);

#if 1 /// elogind needs an Add-On for sleep configuration
        elogind_manager_reset_config(m);
#endif // 1
        log_debug_elogind("%s", "Starting manager...");
        r = manager_startup(m);
        if (r < 0)
                return log_error_errno(r, "Failed to fully start up daemon: %m");

#if 1 /// Do not tell anybody if elogind is restarted through an INTerrupt
        if (m->do_interrupt) {
                log_debug_elogind("elogind interrupted (PID "
                PID_FMT
                "), going down silently...", getpid_cached());
        } else {
#endif /// 1
#if 1 /// extra bracket for elogind here
        }
#endif // 1

        notify_message = notify_start(NOTIFY_READY, NOTIFY_STOPPING);
        return manager_run(m);
}

DEFINE_MAIN_FUNCTION(run);
