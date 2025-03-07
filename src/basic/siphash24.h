/* SPDX-License-Identifier: CC0-1.0 */

#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "string-util.h"
#include "time-util.h"

struct siphash {
        uint64_t v0;
        uint64_t v1;
        uint64_t v2;
        uint64_t v3;
        uint64_t padding;
        size_t inlen;
};

void siphash24_init(struct siphash *state, const uint8_t k[static 16]);
void siphash24_compress(const void *in, size_t inlen, struct siphash *state);
#if 0 /// UNNEEDED by elogind
#define siphash24_compress_byte(byte, state) siphash24_compress((const uint8_t[]) { (byte) }, 1, (state))
#endif // 0

static inline void siphash24_compress_boolean(bool in, struct siphash *state) {
        uint8_t i = in;

        siphash24_compress(&i, sizeof i, state);
}

static inline void siphash24_compress_usec_t(usec_t in, struct siphash *state) {
        siphash24_compress(&in, sizeof in, state);
}

static inline void siphash24_compress_safe(const void *in, size_t inlen, struct siphash *state) {
        if (inlen == 0)
                return;

        siphash24_compress(in, inlen, state);
}

static inline void siphash24_compress_string(const char *in, struct siphash *state) {
        siphash24_compress_safe(in, strlen_ptr(in), state);
}

uint64_t siphash24_finalize(struct siphash *state);

uint64_t siphash24(const void *in, size_t inlen, const uint8_t k[static 16]);

#if 0 /// UNNEEDED by elogind
static inline uint64_t siphash24_string(const char *s, const uint8_t k[static 16]) {
        return siphash24(s, strlen(s) + 1, k);
}
#endif // 0
