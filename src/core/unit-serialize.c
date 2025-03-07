/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "bus-util.h"
#include "dbus.h"
#include "fileio-label.h"
#include "fileio.h"
#include "format-util.h"
#include "parse-util.h"
#include "serialize.h"
#include "string-table.h"
#include "unit-serialize.h"
#include "user-util.h"

static int serialize_cgroup_mask(FILE *f, const char *key, CGroupMask mask) {
        _cleanup_free_ char *s = NULL;
        int r;

        assert(f);
        assert(key);

        if (mask == 0)
                return 0;

        r = cg_mask_to_string(mask, &s);
        if (r < 0)
                return log_error_errno(r, "Failed to format cgroup mask: %m");

        return serialize_item(f, key, s);
}

/* Make sure out values fit in the bitfield. */
assert_cc(_UNIT_MARKER_MAX <= sizeof(((Unit){}).markers) * 8);

static int serialize_markers(FILE *f, unsigned markers) {
        assert(f);

        if (markers == 0)
                return 0;

        fputs("markers=", f);
        for (UnitMarker m = 0; m < _UNIT_MARKER_MAX; m++)
                if (FLAGS_SET(markers, 1u << m))
                        fputs(unit_marker_to_string(m), f);
        fputc('\n', f);
        return 0;
}

static int deserialize_markers(Unit *u, const char *value) {
        assert(u);
        assert(value);
        int r;

        for (const char *p = value;;) {
                _cleanup_free_ char *word = NULL;

                r = extract_first_word(&p, &word, NULL, 0);
                if (r <= 0)
                        return r;

                UnitMarker m = unit_marker_from_string(word);
                if (m < 0) {
                        log_unit_debug_errno(u, m, "Unknown unit marker \"%s\", ignoring.", word);
                        continue;
                }

                u->markers |= 1u << m;
        }
}

static const char *const ip_accounting_metric_field[_CGROUP_IP_ACCOUNTING_METRIC_MAX] = {
        [CGROUP_IP_INGRESS_BYTES] = "ip-accounting-ingress-bytes",
        [CGROUP_IP_INGRESS_PACKETS] = "ip-accounting-ingress-packets",
        [CGROUP_IP_EGRESS_BYTES] = "ip-accounting-egress-bytes",
        [CGROUP_IP_EGRESS_PACKETS] = "ip-accounting-egress-packets",
};

static const char *const io_accounting_metric_field_base[_CGROUP_IO_ACCOUNTING_METRIC_MAX] = {
        [CGROUP_IO_READ_BYTES] = "io-accounting-read-bytes-base",
        [CGROUP_IO_WRITE_BYTES] = "io-accounting-write-bytes-base",
        [CGROUP_IO_READ_OPERATIONS] = "io-accounting-read-operations-base",
        [CGROUP_IO_WRITE_OPERATIONS] = "io-accounting-write-operations-base",
};

static const char *const io_accounting_metric_field_last[_CGROUP_IO_ACCOUNTING_METRIC_MAX] = {
        [CGROUP_IO_READ_BYTES] = "io-accounting-read-bytes-last",
        [CGROUP_IO_WRITE_BYTES] = "io-accounting-write-bytes-last",
        [CGROUP_IO_READ_OPERATIONS] = "io-accounting-read-operations-last",
        [CGROUP_IO_WRITE_OPERATIONS] = "io-accounting-write-operations-last",
};

int unit_serialize(Unit *u, FILE *f, FDSet *fds, bool serialize_jobs) {
        int r;

        assert(u);
        assert(f);
        assert(fds);

        if (unit_can_serialize(u)) {
                r = UNIT_VTABLE(u)->serialize(u, f, fds);
                if (r < 0)
                        return r;
        }

        (void) serialize_dual_timestamp(f, "state-change-timestamp", &u->state_change_timestamp);

        (void) serialize_dual_timestamp(f, "inactive-exit-timestamp", &u->inactive_exit_timestamp);
        (void) serialize_dual_timestamp(f, "active-enter-timestamp", &u->active_enter_timestamp);
        (void) serialize_dual_timestamp(f, "active-exit-timestamp", &u->active_exit_timestamp);
        (void) serialize_dual_timestamp(f, "inactive-enter-timestamp", &u->inactive_enter_timestamp);

        (void) serialize_dual_timestamp(f, "condition-timestamp", &u->condition_timestamp);
        (void) serialize_dual_timestamp(f, "assert-timestamp", &u->assert_timestamp);

        if (dual_timestamp_is_set(&u->condition_timestamp))
                (void) serialize_bool(f, "condition-result", u->condition_result);

        if (dual_timestamp_is_set(&u->assert_timestamp))
                (void) serialize_bool(f, "assert-result", u->assert_result);

        (void) serialize_bool(f, "transient", u->transient);
        (void) serialize_bool(f, "in-audit", u->in_audit);

        (void) serialize_bool(f, "exported-invocation-id", u->exported_invocation_id);
        (void) serialize_bool(f, "exported-log-level-max", u->exported_log_level_max);
        (void) serialize_bool(f, "exported-log-extra-fields", u->exported_log_extra_fields);
        (void) serialize_bool(f, "exported-log-rate-limit-interval", u->exported_log_ratelimit_interval);
        (void) serialize_bool(f, "exported-log-rate-limit-burst", u->exported_log_ratelimit_burst);

        (void) serialize_item_format(f, "cpu-usage-base", "%" PRIu64, u->cpu_usage_base);
        if (u->cpu_usage_last != NSEC_INFINITY)
                (void) serialize_item_format(f, "cpu-usage-last", "%" PRIu64, u->cpu_usage_last);

        if (u->managed_oom_kill_last > 0)
                (void) serialize_item_format(f, "managed-oom-kill-last", "%" PRIu64, u->managed_oom_kill_last);

        if (u->oom_kill_last > 0)
                (void) serialize_item_format(f, "oom-kill-last", "%" PRIu64, u->oom_kill_last);

        for (CGroupIOAccountingMetric im = 0; im < _CGROUP_IO_ACCOUNTING_METRIC_MAX; im++) {
                (void) serialize_item_format(f, io_accounting_metric_field_base[im], "%" PRIu64, u->io_accounting_base[im]);

                if (u->io_accounting_last[im] != UINT64_MAX)
                        (void) serialize_item_format(f, io_accounting_metric_field_last[im], "%" PRIu64, u->io_accounting_last[im]);
        }

        if (u->cgroup_path)
                (void) serialize_item(f, "cgroup", u->cgroup_path);

        (void) serialize_bool(f, "cgroup-realized", u->cgroup_realized);
        (void) serialize_cgroup_mask(f, "cgroup-realized-mask", u->cgroup_realized_mask);
        (void) serialize_cgroup_mask(f, "cgroup-enabled-mask", u->cgroup_enabled_mask);
        (void) serialize_cgroup_mask(f, "cgroup-invalidated-mask", u->cgroup_invalidated_mask);

        if (uid_is_valid(u->ref_uid))
                (void) serialize_item_format(f, "ref-uid", UID_FMT, u->ref_uid);
        if (gid_is_valid(u->ref_gid))
                (void) serialize_item_format(f, "ref-gid", GID_FMT, u->ref_gid);

        if (!sd_id128_is_null(u->invocation_id))
                (void) serialize_item_format(f, "invocation-id", SD_ID128_FORMAT_STR, SD_ID128_FORMAT_VAL(u->invocation_id));

        (void) serialize_item_format(f, "freezer-state", "%s", freezer_state_to_string(unit_freezer_state(u)));
        (void) serialize_markers(f, u->markers);

        bus_track_serialize(u->bus_track, f, "ref");

        for (CGroupIPAccountingMetric m = 0; m < _CGROUP_IP_ACCOUNTING_METRIC_MAX; m++) {
                uint64_t v;

                r = unit_get_ip_accounting(u, m, &v);
                if (r >= 0)
                        (void) serialize_item_format(f, ip_accounting_metric_field[m], "%" PRIu64, v);
        }

        if (serialize_jobs) {
                if (u->job) {
                        fputs("job\n", f);
                        job_serialize(u->job, f);
                }

                if (u->nop_job) {
                        fputs("job\n", f);
                        job_serialize(u->nop_job, f);
                }
        }

        /* End marker */
        fputc('\n', f);
        return 0;
}

static int unit_deserialize_job(Unit *u, FILE *f) {
        _cleanup_(job_freep) Job *j = NULL;
        int r;

        assert(u);
        assert(f);

        j = job_new_raw(u);
        if (!j)
                return log_oom();

        r = job_deserialize(j, f);
        if (r < 0)
                return r;

        r = job_install_deserialized(j);
        if (r < 0)
                return r;

        TAKE_PTR(j);
        return 0;
}

#define MATCH_DESERIALIZE(key, l, v, parse_func, target)                \
        ({                                                              \
                bool _deserialize_matched = streq(l, key);              \
                if (_deserialize_matched) {                             \
                        int _deserialize_r = parse_func(v);             \
                        if (_deserialize_r < 0)                         \
                                log_unit_debug_errno(u, _deserialize_r, \
                                                     "Failed to parse \"%s=%s\", ignoring.", l, v); \
                        else                                            \
                                target = _deserialize_r;                \
                };                                                      \
                _deserialize_matched;                                   \
        })

#define MATCH_DESERIALIZE_IMMEDIATE(key, l, v, parse_func, target)      \
        ({                                                              \
                bool _deserialize_matched = streq(l, key);              \
                if (_deserialize_matched) {                             \
                        int _deserialize_r = parse_func(v, &target);    \
                        if (_deserialize_r < 0)                         \
                                log_unit_debug_errno(u, _deserialize_r, \
                                                     "Failed to parse \"%s=%s\", ignoring", l, v); \
                };                                                      \
                _deserialize_matched;                                   \
        })

int unit_deserialize(Unit *u, FILE *f, FDSet *fds) {
        int r;

        assert(u);
        assert(f);
        assert(fds);

        for (;;) {
                _cleanup_free_ char *line = NULL;
                char *l, *v;
                ssize_t m;
                size_t k;

                r = read_line(f, LONG_LINE_MAX, &line);
                if (r < 0)
                        return log_error_errno(r, "Failed to read serialization line: %m");
                if (r == 0) /* eof */
                        break;

                l = strstrip(line);
                if (isempty(l)) /* End marker */
                        break;

                k = strcspn(l, "=");

                if (l[k] == '=') {
                        l[k] = 0;
                        v = l+k+1;
                } else
                        v = l+k;

                if (streq(l, "job")) {
                        if (v[0] == '\0') {
                                /* New-style serialized job */
                                r = unit_deserialize_job(u, f);
                                if (r < 0)
                                        return r;
                        } else  /* Legacy for pre-44 */
                                log_unit_warning(u, "Update from too old elogind versions are unsupported, cannot deserialize job: %s", v);
                        continue;
                } else if (streq(l, "state-change-timestamp")) {
                        (void) deserialize_dual_timestamp(v, &u->state_change_timestamp);
                        continue;
                } else if (streq(l, "inactive-exit-timestamp")) {
                        (void) deserialize_dual_timestamp(v, &u->inactive_exit_timestamp);
                        continue;
                } else if (streq(l, "active-enter-timestamp")) {
                        (void) deserialize_dual_timestamp(v, &u->active_enter_timestamp);
                        continue;
                } else if (streq(l, "active-exit-timestamp")) {
                        (void) deserialize_dual_timestamp(v, &u->active_exit_timestamp);
                        continue;
                } else if (streq(l, "inactive-enter-timestamp")) {
                        (void) deserialize_dual_timestamp(v, &u->inactive_enter_timestamp);
                        continue;
                } else if (streq(l, "condition-timestamp")) {
                        (void) deserialize_dual_timestamp(v, &u->condition_timestamp);
                        continue;
                } else if (streq(l, "assert-timestamp")) {
                        (void) deserialize_dual_timestamp(v, &u->assert_timestamp);
                        continue;

                } else if (MATCH_DESERIALIZE("condition-result", l, v, parse_boolean, u->condition_result))
                        continue;

                else if (MATCH_DESERIALIZE("assert-result", l, v, parse_boolean, u->assert_result))
                        continue;

                else if (MATCH_DESERIALIZE("transient", l, v, parse_boolean, u->transient))
                        continue;

                else if (MATCH_DESERIALIZE("in-audit", l, v, parse_boolean, u->in_audit))
                        continue;

                else if (MATCH_DESERIALIZE("exported-invocation-id", l, v, parse_boolean, u->exported_invocation_id))
                        continue;

                else if (MATCH_DESERIALIZE("exported-log-level-max", l, v, parse_boolean, u->exported_log_level_max))
                        continue;

                else if (MATCH_DESERIALIZE("exported-log-extra-fields", l, v, parse_boolean, u->exported_log_extra_fields))
                        continue;

                else if (MATCH_DESERIALIZE("exported-log-rate-limit-interval", l, v, parse_boolean, u->exported_log_ratelimit_interval))
                        continue;

                else if (MATCH_DESERIALIZE("exported-log-rate-limit-burst", l, v, parse_boolean, u->exported_log_ratelimit_burst))
                        continue;

                else if (MATCH_DESERIALIZE_IMMEDIATE("cpu-usage-base", l, v, safe_atou64, u->cpu_usage_base) ||
                         MATCH_DESERIALIZE_IMMEDIATE("cpuacct-usage-base", l, v, safe_atou64, u->cpu_usage_base))
                        continue;

                else if (MATCH_DESERIALIZE_IMMEDIATE("cpu-usage-last", l, v, safe_atou64, u->cpu_usage_last))
                        continue;

                else if (MATCH_DESERIALIZE_IMMEDIATE("managed-oom-kill-last", l, v, safe_atou64, u->managed_oom_kill_last))
                        continue;

                else if (MATCH_DESERIALIZE_IMMEDIATE("oom-kill-last", l, v, safe_atou64, u->oom_kill_last))
                        continue;

                else if (streq(l, "cgroup")) {
                        r = unit_set_cgroup_path(u, v);
                        if (r < 0)
                                log_unit_debug_errno(u, r, "Failed to set cgroup path %s, ignoring: %m", v);

                        (void) unit_watch_cgroup(u);
                        (void) unit_watch_cgroup_memory(u);

                        continue;

                } else if (MATCH_DESERIALIZE("cgroup-realized", l, v, parse_boolean, u->cgroup_realized))
                        continue;

                else if (MATCH_DESERIALIZE_IMMEDIATE("cgroup-realized-mask", l, v, cg_mask_from_string, u->cgroup_realized_mask))
                        continue;

                else if (MATCH_DESERIALIZE_IMMEDIATE("cgroup-enabled-mask", l, v, cg_mask_from_string, u->cgroup_enabled_mask))
                        continue;

                else if (MATCH_DESERIALIZE_IMMEDIATE("cgroup-invalidated-mask", l, v, cg_mask_from_string, u->cgroup_invalidated_mask))
                        continue;

                else if (streq(l, "ref-uid")) {
                        uid_t uid;

                        r = parse_uid(v, &uid);
                        if (r < 0)
                                log_unit_debug(u, "Failed to parse \"%s=%s\", ignoring.", l, v);
                        else
                                unit_ref_uid_gid(u, uid, GID_INVALID);
                        continue;

                } else if (streq(l, "ref-gid")) {
                        gid_t gid;

                        r = parse_gid(v, &gid);
                        if (r < 0)
                                log_unit_debug(u, "Failed to parse \"%s=%s\", ignoring.", l, v);
                        else
                                unit_ref_uid_gid(u, UID_INVALID, gid);
                        continue;

                } else if (streq(l, "ref")) {
                        r = strv_extend(&u->deserialized_refs, v);
                        if (r < 0)
                                return log_oom();
                        continue;

                } else if (streq(l, "invocation-id")) {
                        sd_id128_t id;

                        r = sd_id128_from_string(v, &id);
                        if (r < 0)
                                log_unit_debug(u, "Failed to parse \"%s=%s\", ignoring.", l, v);
                        else {
                                r = unit_set_invocation_id(u, id);
                                if (r < 0)
                                        log_unit_warning_errno(u, r, "Failed to set invocation ID for unit: %m");
                        }

                        continue;

                } else if (MATCH_DESERIALIZE("freezer-state", l, v, freezer_state_from_string, u->freezer_state))
                        continue;

                else if (streq(l, "markers")) {
                        r = deserialize_markers(u, v);
                        if (r < 0)
                                log_unit_debug_errno(u, r, "Failed to deserialize \"%s=%s\", ignoring: %m", l, v);
                        continue;
                }

                /* Check if this is an IP accounting metric serialization field */
                m = string_table_lookup(ip_accounting_metric_field, ELEMENTSOF(ip_accounting_metric_field), l);
                if (m >= 0) {
                        uint64_t c;

                        r = safe_atou64(v, &c);
                        if (r < 0)
                                log_unit_debug(u, "Failed to parse IP accounting value %s, ignoring.", v);
                        else
                                u->ip_accounting_extra[m] = c;
                        continue;
                }

                m = string_table_lookup(io_accounting_metric_field_base, ELEMENTSOF(io_accounting_metric_field_base), l);
                if (m >= 0) {
                        uint64_t c;

                        r = safe_atou64(v, &c);
                        if (r < 0)
                                log_unit_debug(u, "Failed to parse IO accounting base value %s, ignoring.", v);
                        else
                                u->io_accounting_base[m] = c;
                        continue;
                }

                m = string_table_lookup(io_accounting_metric_field_last, ELEMENTSOF(io_accounting_metric_field_last), l);
                if (m >= 0) {
                        uint64_t c;

                        r = safe_atou64(v, &c);
                        if (r < 0)
                                log_unit_debug(u, "Failed to parse IO accounting last value %s, ignoring.", v);
                        else
                                u->io_accounting_last[m] = c;
                        continue;
                }

                if (unit_can_serialize(u)) {
                        r = exec_runtime_deserialize_compat(u, l, v, fds);
                        if (r < 0) {
                                log_unit_warning(u, "Failed to deserialize runtime parameter '%s', ignoring.", l);
                                continue;
                        }

                        /* Returns positive if key was handled by the call */
                        if (r > 0)
                                continue;

                        r = UNIT_VTABLE(u)->deserialize_item(u, l, v, fds);
                        if (r < 0)
                                log_unit_warning(u, "Failed to deserialize unit parameter '%s', ignoring.", l);
                }
        }

        /* Versions before 228 did not carry a state change timestamp. In this case, take the current
         * time. This is useful, so that timeouts based on this timestamp don't trigger too early, and is
         * in-line with the logic from before 228 where the base for timeouts was not persistent across
         * reboots. */

        if (!dual_timestamp_is_set(&u->state_change_timestamp))
                dual_timestamp_get(&u->state_change_timestamp);

        /* Let's make sure that everything that is deserialized also gets any potential new cgroup settings
         * applied after we are done. For that we invalidate anything already realized, so that we can
         * realize it again. */
        unit_invalidate_cgroup(u, _CGROUP_MASK_ALL);
        unit_invalidate_cgroup_bpf(u);

        return 0;
}

int unit_deserialize_skip(FILE *f) {
        int r;
        assert(f);

        /* Skip serialized data for this unit. We don't know what it is. */

        for (;;) {
                _cleanup_free_ char *line = NULL;
                char *l;

                r = read_line(f, LONG_LINE_MAX, &line);
                if (r < 0)
                        return log_error_errno(r, "Failed to read serialization line: %m");
                if (r == 0)
                        return 0;

                l = strstrip(line);

                /* End marker */
                if (isempty(l))
                        return 1;
        }
}

static void print_unit_dependency_mask(FILE *f, const char *kind, UnitDependencyMask mask, bool *space) {
        const struct {
                UnitDependencyMask mask;
                const char *name;
        } table[] = {
                { UNIT_DEPENDENCY_FILE,               "file"               },
                { UNIT_DEPENDENCY_IMPLICIT,           "implicit"           },
                { UNIT_DEPENDENCY_DEFAULT,            "default"            },
                { UNIT_DEPENDENCY_UDEV,               "udev"               },
                { UNIT_DEPENDENCY_PATH,               "path"               },
                { UNIT_DEPENDENCY_MOUNTINFO_IMPLICIT, "mountinfo-implicit" },
                { UNIT_DEPENDENCY_MOUNTINFO_DEFAULT,  "mountinfo-default"  },
                { UNIT_DEPENDENCY_PROC_SWAP,          "proc-swap"          },
        };

        assert(f);
        assert(kind);
        assert(space);

        for (size_t i = 0; i < ELEMENTSOF(table); i++) {

                if (mask == 0)
                        break;

                if (FLAGS_SET(mask, table[i].mask)) {
                        if (*space)
                                fputc(' ', f);
                        else
                                *space = true;

                        fputs(kind, f);
                        fputs("-", f);
                        fputs(table[i].name, f);

                        mask &= ~table[i].mask;
                }
        }

        assert(mask == 0);
}

void unit_dump(Unit *u, FILE *f, const char *prefix) {
        char *t, **j;
        const char *prefix2;
        char timestamp[5][FORMAT_TIMESTAMP_MAX], timespan[FORMAT_TIMESPAN_MAX];
        Unit *following;
        _cleanup_set_free_ Set *following_set = NULL;
        CGroupMask m;
        int r;

        assert(u);
        assert(u->type >= 0);

        prefix = strempty(prefix);
        prefix2 = strjoina(prefix, "\t");

        fprintf(f,
                "%s-> Unit %s:\n",
                prefix, u->id);

        SET_FOREACH(t, u->aliases)
                fprintf(f, "%s\tAlias: %s\n", prefix, t);

        fprintf(f,
                "%s\tDescription: %s\n"
                "%s\tInstance: %s\n"
                "%s\tUnit Load State: %s\n"
                "%s\tUnit Active State: %s\n"
                "%s\tState Change Timestamp: %s\n"
                "%s\tInactive Exit Timestamp: %s\n"
                "%s\tActive Enter Timestamp: %s\n"
                "%s\tActive Exit Timestamp: %s\n"
                "%s\tInactive Enter Timestamp: %s\n"
                "%s\tMay GC: %s\n"
                "%s\tNeed Daemon Reload: %s\n"
                "%s\tTransient: %s\n"
                "%s\tPerpetual: %s\n"
                "%s\tGarbage Collection Mode: %s\n"
                "%s\tSlice: %s\n"
                "%s\tCGroup: %s\n"
                "%s\tCGroup realized: %s\n",
                prefix, unit_description(u),
                prefix, strna(u->instance),
                prefix, unit_load_state_to_string(u->load_state),
                prefix, unit_active_state_to_string(unit_active_state(u)),
                prefix, strna(format_timestamp(timestamp[0], sizeof(timestamp[0]), u->state_change_timestamp.realtime)),
                prefix, strna(format_timestamp(timestamp[1], sizeof(timestamp[1]), u->inactive_exit_timestamp.realtime)),
                prefix, strna(format_timestamp(timestamp[2], sizeof(timestamp[2]), u->active_enter_timestamp.realtime)),
                prefix, strna(format_timestamp(timestamp[3], sizeof(timestamp[3]), u->active_exit_timestamp.realtime)),
                prefix, strna(format_timestamp(timestamp[4], sizeof(timestamp[4]), u->inactive_enter_timestamp.realtime)),
                prefix, yes_no(unit_may_gc(u)),
                prefix, yes_no(unit_need_daemon_reload(u)),
                prefix, yes_no(u->transient),
                prefix, yes_no(u->perpetual),
                prefix, collect_mode_to_string(u->collect_mode),
                prefix, strna(unit_slice_name(u)),
                prefix, strna(u->cgroup_path),
                prefix, yes_no(u->cgroup_realized));

        if (u->markers != 0) {
                fprintf(f, "%s\tMarkers:", prefix);

                for (UnitMarker marker = 0; marker < _UNIT_MARKER_MAX; marker++)
                        if (FLAGS_SET(u->markers, 1u << marker))
                                fprintf(f, " %s", unit_marker_to_string(marker));
                fputs("\n", f);
        }

        if (u->cgroup_realized_mask != 0) {
                _cleanup_free_ char *s = NULL;
                (void) cg_mask_to_string(u->cgroup_realized_mask, &s);
                fprintf(f, "%s\tCGroup realized mask: %s\n", prefix, strnull(s));
        }

        if (u->cgroup_enabled_mask != 0) {
                _cleanup_free_ char *s = NULL;
                (void) cg_mask_to_string(u->cgroup_enabled_mask, &s);
                fprintf(f, "%s\tCGroup enabled mask: %s\n", prefix, strnull(s));
        }

        m = unit_get_own_mask(u);
        if (m != 0) {
                _cleanup_free_ char *s = NULL;
                (void) cg_mask_to_string(m, &s);
                fprintf(f, "%s\tCGroup own mask: %s\n", prefix, strnull(s));
        }

        m = unit_get_members_mask(u);
        if (m != 0) {
                _cleanup_free_ char *s = NULL;
                (void) cg_mask_to_string(m, &s);
                fprintf(f, "%s\tCGroup members mask: %s\n", prefix, strnull(s));
        }

        m = unit_get_delegate_mask(u);
        if (m != 0) {
                _cleanup_free_ char *s = NULL;
                (void) cg_mask_to_string(m, &s);
                fprintf(f, "%s\tCGroup delegate mask: %s\n", prefix, strnull(s));
        }

        if (!sd_id128_is_null(u->invocation_id))
                fprintf(f, "%s\tInvocation ID: " SD_ID128_FORMAT_STR "\n",
                        prefix, SD_ID128_FORMAT_VAL(u->invocation_id));

        STRV_FOREACH(j, u->documentation)
                fprintf(f, "%s\tDocumentation: %s\n", prefix, *j);

        following = unit_following(u);
        if (following)
                fprintf(f, "%s\tFollowing: %s\n", prefix, following->id);

        r = unit_following_set(u, &following_set);
        if (r >= 0) {
                Unit *other;

                SET_FOREACH(other, following_set)
                        fprintf(f, "%s\tFollowing Set Member: %s\n", prefix, other->id);
        }

        if (u->fragment_path)
                fprintf(f, "%s\tFragment Path: %s\n", prefix, u->fragment_path);

        if (u->source_path)
                fprintf(f, "%s\tSource Path: %s\n", prefix, u->source_path);

        STRV_FOREACH(j, u->dropin_paths)
                fprintf(f, "%s\tDropIn Path: %s\n", prefix, *j);

        if (u->failure_action != EMERGENCY_ACTION_NONE)
                fprintf(f, "%s\tFailure Action: %s\n", prefix, emergency_action_to_string(u->failure_action));
        if (u->failure_action_exit_status >= 0)
                fprintf(f, "%s\tFailure Action Exit Status: %i\n", prefix, u->failure_action_exit_status);
        if (u->success_action != EMERGENCY_ACTION_NONE)
                fprintf(f, "%s\tSuccess Action: %s\n", prefix, emergency_action_to_string(u->success_action));
        if (u->success_action_exit_status >= 0)
                fprintf(f, "%s\tSuccess Action Exit Status: %i\n", prefix, u->success_action_exit_status);

        if (u->job_timeout != USEC_INFINITY)
                fprintf(f, "%s\tJob Timeout: %s\n", prefix, format_timespan(timespan, sizeof(timespan), u->job_timeout, 0));

        if (u->job_timeout_action != EMERGENCY_ACTION_NONE)
                fprintf(f, "%s\tJob Timeout Action: %s\n", prefix, emergency_action_to_string(u->job_timeout_action));

        if (u->job_timeout_reboot_arg)
                fprintf(f, "%s\tJob Timeout Reboot Argument: %s\n", prefix, u->job_timeout_reboot_arg);

        condition_dump_list(u->conditions, f, prefix, condition_type_to_string);
        condition_dump_list(u->asserts, f, prefix, assert_type_to_string);

        if (dual_timestamp_is_set(&u->condition_timestamp))
                fprintf(f,
                        "%s\tCondition Timestamp: %s\n"
                        "%s\tCondition Result: %s\n",
                        prefix, strna(format_timestamp(timestamp[0], sizeof(timestamp[0]), u->condition_timestamp.realtime)),
                        prefix, yes_no(u->condition_result));

        if (dual_timestamp_is_set(&u->assert_timestamp))
                fprintf(f,
                        "%s\tAssert Timestamp: %s\n"
                        "%s\tAssert Result: %s\n",
                        prefix, strna(format_timestamp(timestamp[0], sizeof(timestamp[0]), u->assert_timestamp.realtime)),
                        prefix, yes_no(u->assert_result));

        for (UnitDependency d = 0; d < _UNIT_DEPENDENCY_MAX; d++) {
                UnitDependencyInfo di;
                Unit *other;

                HASHMAP_FOREACH_KEY(di.data, other, u->dependencies[d]) {
                        bool space = false;

                        fprintf(f, "%s\t%s: %s (", prefix, unit_dependency_to_string(d), other->id);

                        print_unit_dependency_mask(f, "origin", di.origin_mask, &space);
                        print_unit_dependency_mask(f, "destination", di.destination_mask, &space);

                        fputs(")\n", f);
                }
        }

        if (!hashmap_isempty(u->requires_mounts_for)) {
                UnitDependencyInfo di;
                const char *path;

                HASHMAP_FOREACH_KEY(di.data, path, u->requires_mounts_for) {
                        bool space = false;

                        fprintf(f, "%s\tRequiresMountsFor: %s (", prefix, path);

                        print_unit_dependency_mask(f, "origin", di.origin_mask, &space);
                        print_unit_dependency_mask(f, "destination", di.destination_mask, &space);

                        fputs(")\n", f);
                }
        }

        if (u->load_state == UNIT_LOADED) {

                fprintf(f,
                        "%s\tStopWhenUnneeded: %s\n"
                        "%s\tRefuseManualStart: %s\n"
                        "%s\tRefuseManualStop: %s\n"
                        "%s\tDefaultDependencies: %s\n"
                        "%s\tOnFailureJobMode: %s\n"
                        "%s\tIgnoreOnIsolate: %s\n",
                        prefix, yes_no(u->stop_when_unneeded),
                        prefix, yes_no(u->refuse_manual_start),
                        prefix, yes_no(u->refuse_manual_stop),
                        prefix, yes_no(u->default_dependencies),
                        prefix, job_mode_to_string(u->on_failure_job_mode),
                        prefix, yes_no(u->ignore_on_isolate));

                if (UNIT_VTABLE(u)->dump)
                        UNIT_VTABLE(u)->dump(u, f, prefix2);

        } else if (u->load_state == UNIT_MERGED)
                fprintf(f,
                        "%s\tMerged into: %s\n",
                        prefix, u->merged_into->id);
        else if (u->load_state == UNIT_ERROR)
                fprintf(f, "%s\tLoad Error Code: %s\n", prefix, strerror_safe(u->load_error));

        for (const char *n = sd_bus_track_first(u->bus_track); n; n = sd_bus_track_next(u->bus_track))
                fprintf(f, "%s\tBus Ref: %s\n", prefix, n);

        if (u->job)
                job_dump(u->job, f, prefix2);

        if (u->nop_job)
                job_dump(u->nop_job, f, prefix2);
}
