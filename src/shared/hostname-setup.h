/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>
#include <stdio.h>

typedef enum HostnameSource {
        HOSTNAME_STATIC,     /* from /etc/hostname */
        HOSTNAME_TRANSIENT,  /* a transient hostname set through elogind, hostnamed, the container manager, or otherwise */
        HOSTNAME_DEFAULT,    /* the os-release default or the compiled-in fallback were used */
        _HOSTNAME_INVALID = -EINVAL,
} HostnameSource;

const char* hostname_source_to_string(HostnameSource source) _const_;
HostnameSource hostname_source_from_string(const char *str) _pure_;

int sethostname_idempotent(const char *s);

int shorten_overlong(const char *s, char **ret);

int read_etc_hostname_stream(FILE *f, char **ret);
int read_etc_hostname(const char *path, char **ret);

bool get_hostname_filtered(char ret[static HOST_NAME_MAX + 1]);
void hostname_update_source_hint(const char *hostname, HostnameSource source);
#if 0 /// UNNEEDED by elogind
int hostname_setup(bool really);
#endif // 0
