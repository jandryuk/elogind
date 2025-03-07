/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef uint64_t usec_t;
typedef uint64_t nsec_t;

#define PRI_NSEC PRIu64
#define PRI_USEC PRIu64
#define NSEC_FMT "%" PRI_NSEC
#define USEC_FMT "%" PRI_USEC

#include "macro.h"

typedef struct dual_timestamp {
        usec_t realtime;
        usec_t monotonic;
} dual_timestamp;

typedef struct triple_timestamp {
        usec_t realtime;
        usec_t monotonic;
        usec_t boottime;
} triple_timestamp;

typedef enum TimestampStyle {
        TIMESTAMP_PRETTY,
        TIMESTAMP_US,
        TIMESTAMP_UTC,
        TIMESTAMP_US_UTC,
        _TIMESTAMP_STYLE_MAX,
        _TIMESTAMP_STYLE_INVALID = -EINVAL,
} TimestampStyle;

#define USEC_INFINITY ((usec_t) UINT64_MAX)
#define NSEC_INFINITY ((nsec_t) UINT64_MAX)

#define MSEC_PER_SEC  1000ULL
#define USEC_PER_SEC  ((usec_t) 1000000ULL)
#define USEC_PER_MSEC ((usec_t) 1000ULL)
#define NSEC_PER_SEC  ((nsec_t) 1000000000ULL)
#define NSEC_PER_MSEC ((nsec_t) 1000000ULL)
#define NSEC_PER_USEC ((nsec_t) 1000ULL)

#define USEC_PER_MINUTE ((usec_t) (60ULL*USEC_PER_SEC))
#define NSEC_PER_MINUTE ((nsec_t) (60ULL*NSEC_PER_SEC))
#define USEC_PER_HOUR ((usec_t) (60ULL*USEC_PER_MINUTE))
#define NSEC_PER_HOUR ((nsec_t) (60ULL*NSEC_PER_MINUTE))
#define USEC_PER_DAY ((usec_t) (24ULL*USEC_PER_HOUR))
#define NSEC_PER_DAY ((nsec_t) (24ULL*NSEC_PER_HOUR))
#define USEC_PER_WEEK ((usec_t) (7ULL*USEC_PER_DAY))
#define NSEC_PER_WEEK ((nsec_t) (7ULL*NSEC_PER_DAY))
#define USEC_PER_MONTH ((usec_t) (2629800ULL*USEC_PER_SEC))
#define NSEC_PER_MONTH ((nsec_t) (2629800ULL*NSEC_PER_SEC))
#define USEC_PER_YEAR ((usec_t) (31557600ULL*USEC_PER_SEC))
#define NSEC_PER_YEAR ((nsec_t) (31557600ULL*NSEC_PER_SEC))

/* We assume a maximum timezone length of 6. TZNAME_MAX is not defined on Linux, but glibc internally initializes this
 * to 6. Let's rely on that. */
#define FORMAT_TIMESTAMP_MAX (3U+1U+10U+1U+8U+1U+6U+1U+6U+1U)
#define FORMAT_TIMESTAMP_WIDTH 28U /* when outputting, assume this width */
#define FORMAT_TIMESTAMP_RELATIVE_MAX 256U
#define FORMAT_TIMESPAN_MAX 64U

#define TIME_T_MAX (time_t)((UINTMAX_C(1) << ((sizeof(time_t) << 3) - 1)) - 1)

#define DUAL_TIMESTAMP_NULL ((struct dual_timestamp) {})
#define TRIPLE_TIMESTAMP_NULL ((struct triple_timestamp) {})

usec_t now(clockid_t clock);
#if 0 /// UNNEEDED by elogind
nsec_t now_nsec(clockid_t clock);
#endif // 0

usec_t map_clock_usec(usec_t from, clockid_t from_clock, clockid_t to_clock);

dual_timestamp* dual_timestamp_get(dual_timestamp *ts);
dual_timestamp* dual_timestamp_from_realtime(dual_timestamp *ts, usec_t u);
#if 0 /// UNNEEDED by elogind
dual_timestamp* dual_timestamp_from_monotonic(dual_timestamp *ts, usec_t u);
dual_timestamp* dual_timestamp_from_boottime_or_monotonic(dual_timestamp *ts, usec_t u);
#endif // 0

triple_timestamp* triple_timestamp_get(triple_timestamp *ts);
triple_timestamp* triple_timestamp_from_realtime(triple_timestamp *ts, usec_t u);

#define DUAL_TIMESTAMP_HAS_CLOCK(clock)                               \
        IN_SET(clock, CLOCK_REALTIME, CLOCK_REALTIME_ALARM, CLOCK_MONOTONIC)

#define TRIPLE_TIMESTAMP_HAS_CLOCK(clock)                               \
        IN_SET(clock, CLOCK_REALTIME, CLOCK_REALTIME_ALARM, CLOCK_MONOTONIC, CLOCK_BOOTTIME, CLOCK_BOOTTIME_ALARM)

static inline bool timestamp_is_set(usec_t timestamp) {
        return timestamp > 0 && timestamp != USEC_INFINITY;
}

static inline bool dual_timestamp_is_set(const dual_timestamp *ts) {
        return timestamp_is_set(ts->realtime) ||
               timestamp_is_set(ts->monotonic);
}

static inline bool triple_timestamp_is_set(const triple_timestamp *ts) {
        return timestamp_is_set(ts->realtime) ||
               timestamp_is_set(ts->monotonic) ||
               timestamp_is_set(ts->boottime);
}

usec_t triple_timestamp_by_clock(triple_timestamp *ts, clockid_t clock);

usec_t timespec_load(const struct timespec *ts) _pure_;
#if 0 /// UNNEEDED by elogind
nsec_t timespec_load_nsec(const struct timespec *ts) _pure_;
#endif // 0
struct timespec *timespec_store(struct timespec *ts, usec_t u);
struct timespec *timespec_store_nsec(struct timespec *ts, nsec_t n);

usec_t timeval_load(const struct timeval *tv) _pure_;
struct timeval *timeval_store(struct timeval *tv, usec_t u);

char *format_timestamp_style(char *buf, size_t l, usec_t t, TimestampStyle style);
char *format_timestamp_relative(char *buf, size_t l, usec_t t);
char *format_timespan(char *buf, size_t l, usec_t t, usec_t accuracy);

static inline char *format_timestamp(char *buf, size_t l, usec_t t) {
        return format_timestamp_style(buf, l, t, TIMESTAMP_PRETTY);
}

#if 0 /// UNNEEDED by elogind
int parse_timestamp(const char *t, usec_t *usec);
#endif // 0

int parse_sec(const char *t, usec_t *usec);
#if 0 /// UNNEEDED by elogind
int parse_sec_fix_0(const char *t, usec_t *usec);
int parse_sec_def_infinity(const char *t, usec_t *usec);
#endif // 0
int parse_time(const char *t, usec_t *usec, usec_t default_unit);
#if 0 /// UNNEEDED by elogind
int parse_nsec(const char *t, nsec_t *nsec);

bool ntp_synced(void);

int get_timezones(char ***l);
#endif // 0
bool timezone_is_valid(const char *name, int log_level);

bool clock_boottime_supported(void);
bool clock_supported(clockid_t clock);
clockid_t clock_boottime_or_monotonic(void);

#if 0 /// UNNEEDED by elogind
usec_t usec_shift_clock(usec_t, clockid_t from, clockid_t to);
#endif // 0

#if 0 /// UNNEEDED by elogind
int get_timezone(char **timezone);

time_t mktime_or_timegm(struct tm *tm, bool utc);
#endif // 0
struct tm *localtime_or_gmtime_r(const time_t *t, struct tm *tm, bool utc);

#if 0 /// UNNEEDED by elogind
uint32_t usec_to_jiffies(usec_t usec);
usec_t jiffies_to_usec(uint32_t jiffies);

bool in_utc_timezone(void);
#endif // 0

static inline usec_t usec_add(usec_t a, usec_t b) {

        /* Adds two time values, and makes sure USEC_INFINITY as input results as USEC_INFINITY in output, and doesn't
         * overflow. */

        if (a > USEC_INFINITY - b) /* overflow check */
                return USEC_INFINITY;

        return a + b;
}

static inline usec_t usec_sub_unsigned(usec_t timestamp, usec_t delta) {

        if (timestamp == USEC_INFINITY) /* Make sure infinity doesn't degrade */
                return USEC_INFINITY;
        if (timestamp < delta)
                return 0;

        return timestamp - delta;
}

static inline usec_t usec_sub_signed(usec_t timestamp, int64_t delta) {
        if (delta < 0)
                return usec_add(timestamp, (usec_t) (-delta));
        else
                return usec_sub_unsigned(timestamp, (usec_t) delta);
}

#if SIZEOF_TIME_T == 8
/* The last second we can format is 31. Dec 9999, 1s before midnight, because otherwise we'd enter 5 digit year
 * territory. However, since we want to stay away from this in all timezones we take one day off. */
#define USEC_TIMESTAMP_FORMATTABLE_MAX ((usec_t) 253402214399000000)
#elif SIZEOF_TIME_T == 4
/* With a 32bit time_t we can't go beyond 2038... */
#define USEC_TIMESTAMP_FORMATTABLE_MAX ((usec_t) 2147483647000000)
#else
#error "Yuck, time_t is neither 4 nor 8 bytes wide?"
#endif

#if 0 /// UNNEEDED by elogind
int time_change_fd(void);
#endif // 0

const char* timestamp_style_to_string(TimestampStyle t) _const_;
TimestampStyle timestamp_style_from_string(const char *s) _pure_;
