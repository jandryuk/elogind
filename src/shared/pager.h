/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>

#include "macro.h"

typedef enum PagerFlags {
        PAGER_DISABLE     = 1 << 0,
        PAGER_JUMP_TO_END = 1 << 1,
} PagerFlags;

int pager_open(PagerFlags flags);
void pager_close(void);
bool pager_have(void) _pure_;

#if 0 /// UNNEEDED by elogind
int show_man_page(const char *page, bool null_stdio);
#endif // 0
