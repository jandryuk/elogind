/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "alloc-util.h"
#include "hashmap.h"
#include "log.h"
#include "nulstr-util.h"
#include "stdio-util.h"
#include "string-util.h"
#include "strv.h"
#include "time-util.h"
#include "tests.h"

void test_hashmap_funcs(void);

static void test_hashmap_replace(void) {
        Hashmap *m;
        char *val1, *val2, *val3, *val4, *val5, *r;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);

        val1 = strdup("val1");
        assert_se(val1);
        val2 = strdup("val2");
        assert_se(val2);
        val3 = strdup("val3");
        assert_se(val3);
        val4 = strdup("val4");
        assert_se(val4);
        val5 = strdup("val5");
        assert_se(val5);

        hashmap_put(m, "key 1", val1);
        hashmap_put(m, "key 2", val2);
        hashmap_put(m, "key 3", val3);
        hashmap_put(m, "key 4", val4);

        hashmap_replace(m, "key 3", val1);
        r = hashmap_get(m, "key 3");
        assert_se(streq(r, "val1"));

        hashmap_replace(m, "key 5", val5);
        r = hashmap_get(m, "key 5");
        assert_se(streq(r, "val5"));

        free(val1);
        free(val2);
        free(val3);
        free(val4);
        free(val5);
        hashmap_free(m);
}

static void test_hashmap_copy(void) {
        Hashmap *m, *copy;
        char *val1, *val2, *val3, *val4, *r;

        log_info("/* %s */", __func__);

        val1 = strdup("val1");
        assert_se(val1);
        val2 = strdup("val2");
        assert_se(val2);
        val3 = strdup("val3");
        assert_se(val3);
        val4 = strdup("val4");
        assert_se(val4);

        m = hashmap_new(&string_hash_ops);

        hashmap_put(m, "key 1", val1);
        hashmap_put(m, "key 2", val2);
        hashmap_put(m, "key 3", val3);
        hashmap_put(m, "key 4", val4);

        copy = hashmap_copy(m);

        r = hashmap_get(copy, "key 1");
        assert_se(streq(r, "val1"));
        r = hashmap_get(copy, "key 2");
        assert_se(streq(r, "val2"));
        r = hashmap_get(copy, "key 3");
        assert_se(streq(r, "val3"));
        r = hashmap_get(copy, "key 4");
        assert_se(streq(r, "val4"));

        hashmap_free_free(copy);
        hashmap_free(m);
}

static void test_hashmap_get_strv(void) {
        Hashmap *m;
        char **strv;
        char *val1, *val2, *val3, *val4;

        log_info("/* %s */", __func__);

        val1 = strdup("val1");
        assert_se(val1);
        val2 = strdup("val2");
        assert_se(val2);
        val3 = strdup("val3");
        assert_se(val3);
        val4 = strdup("val4");
        assert_se(val4);

        m = hashmap_new(&string_hash_ops);

        hashmap_put(m, "key 1", val1);
        hashmap_put(m, "key 2", val2);
        hashmap_put(m, "key 3", val3);
        hashmap_put(m, "key 4", val4);

        strv = hashmap_get_strv(m);

#ifndef ORDERED
        strv = strv_sort(strv);
#endif

        assert_se(streq(strv[0], "val1"));
        assert_se(streq(strv[1], "val2"));
        assert_se(streq(strv[2], "val3"));
        assert_se(streq(strv[3], "val4"));

        strv_free(strv);

        hashmap_free(m);
}

static void test_hashmap_move_one(void) {
        Hashmap *m, *n;
        char *val1, *val2, *val3, *val4, *r;

        log_info("/* %s */", __func__);

        val1 = strdup("val1");
        assert_se(val1);
        val2 = strdup("val2");
        assert_se(val2);
        val3 = strdup("val3");
        assert_se(val3);
        val4 = strdup("val4");
        assert_se(val4);

        m = hashmap_new(&string_hash_ops);
        n = hashmap_new(&string_hash_ops);

        hashmap_put(m, "key 1", val1);
        hashmap_put(m, "key 2", val2);
        hashmap_put(m, "key 3", val3);
        hashmap_put(m, "key 4", val4);

        assert_se(hashmap_move_one(n, NULL, "key 3") == -ENOENT);
        assert_se(hashmap_move_one(n, m, "key 5") == -ENOENT);
        assert_se(hashmap_move_one(n, m, "key 3") == 0);
        assert_se(hashmap_move_one(n, m, "key 4") == 0);

        r = hashmap_get(n, "key 3");
        assert_se(r && streq(r, "val3"));
        r = hashmap_get(n, "key 4");
        assert_se(r && streq(r, "val4"));
        r = hashmap_get(m, "key 3");
        assert_se(!r);

        assert_se(hashmap_move_one(n, m, "key 3") == -EEXIST);

        hashmap_free_free(m);
        hashmap_free_free(n);
}

static void test_hashmap_move(void) {
        Hashmap *m, *n;
        char *val1, *val2, *val3, *val4, *r;

        log_info("/* %s */", __func__);

        val1 = strdup("val1");
        assert_se(val1);
        val2 = strdup("val2");
        assert_se(val2);
        val3 = strdup("val3");
        assert_se(val3);
        val4 = strdup("val4");
        assert_se(val4);

        m = hashmap_new(&string_hash_ops);
        n = hashmap_new(&string_hash_ops);

        hashmap_put(n, "key 1", strdup(val1));
        hashmap_put(m, "key 1", val1);
        hashmap_put(m, "key 2", val2);
        hashmap_put(m, "key 3", val3);
        hashmap_put(m, "key 4", val4);

        assert_se(hashmap_move(n, NULL) == 0);
        assert_se(hashmap_move(n, m) == 0);

        assert_se(hashmap_size(m) == 1);
        r = hashmap_get(m, "key 1");
        assert_se(r && streq(r, "val1"));

        r = hashmap_get(n, "key 1");
        assert_se(r && streq(r, "val1"));
        r = hashmap_get(n, "key 2");
        assert_se(r && streq(r, "val2"));
        r = hashmap_get(n, "key 3");
        assert_se(r && streq(r, "val3"));
        r = hashmap_get(n, "key 4");
        assert_se(r && streq(r, "val4"));

        hashmap_free_free(m);
        hashmap_free_free(n);
}

static void test_hashmap_update(void) {
        Hashmap *m;
        char *val1, *val2, *r;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);
        val1 = strdup("old_value");
        assert_se(val1);
        val2 = strdup("new_value");
        assert_se(val2);

        hashmap_put(m, "key 1", val1);
        r = hashmap_get(m, "key 1");
        assert_se(streq(r, "old_value"));

        assert_se(hashmap_update(m, "key 2", val2) == -ENOENT);
        r = hashmap_get(m, "key 1");
        assert_se(streq(r, "old_value"));

        assert_se(hashmap_update(m, "key 1", val2) == 0);
        r = hashmap_get(m, "key 1");
        assert_se(streq(r, "new_value"));

        free(val1);
        free(val2);
        hashmap_free(m);
}

static void test_hashmap_put(void) {
        Hashmap *m = NULL;
        int valid_hashmap_put;
        void *val1 = (void*) "val 1";
        void *val2 = (void*) "val 2";
        _cleanup_free_ char* key1 = NULL;

        log_info("/* %s */", __func__);

        assert_se(hashmap_ensure_allocated(&m, &string_hash_ops) == 1);
        assert_se(m);

        valid_hashmap_put = hashmap_put(m, "key 1", val1);
        assert_se(valid_hashmap_put == 1);
        assert_se(hashmap_put(m, "key 1", val1) == 0);
        assert_se(hashmap_put(m, "key 1", val2) == -EEXIST);
        key1 = strdup("key 1");
        assert_se(hashmap_put(m, key1, val1) == 0);
        assert_se(hashmap_put(m, key1, val2) == -EEXIST);

        hashmap_free(m);
}

static void test_hashmap_remove(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;
        char *r;

        log_info("/* %s */", __func__);

        r = hashmap_remove(NULL, "key 1");
        assert_se(r == NULL);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        r = hashmap_remove(m, "no such key");
        assert_se(r == NULL);

        hashmap_put(m, "key 1", (void*) "val 1");
        hashmap_put(m, "key 2", (void*) "val 2");

        r = hashmap_remove(m, "key 1");
        assert_se(streq(r, "val 1"));

        r = hashmap_get(m, "key 2");
        assert_se(streq(r, "val 2"));
        assert_se(!hashmap_get(m, "key 1"));
}

static void test_hashmap_remove2(void) {
        _cleanup_hashmap_free_free_free_ Hashmap *m = NULL;
        char key1[] = "key 1";
        char key2[] = "key 2";
        char val1[] = "val 1";
        char val2[] = "val 2";
        void *r, *r2;

        log_info("/* %s */", __func__);

        r = hashmap_remove2(NULL, "key 1", &r2);
        assert_se(r == NULL);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        r = hashmap_remove2(m, "no such key", &r2);
        assert_se(r == NULL);

        hashmap_put(m, strdup(key1), strdup(val1));
        hashmap_put(m, strdup(key2), strdup(val2));

        r = hashmap_remove2(m, key1, &r2);
        assert_se(streq(r, val1));
        assert_se(streq(r2, key1));
        free(r);
        free(r2);

        r = hashmap_get(m, key2);
        assert_se(streq(r, val2));
        assert_se(!hashmap_get(m, key1));
}

static void test_hashmap_remove_value(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;
        char *r;

        char val1[] = "val 1";
        char val2[] = "val 2";

        log_info("/* %s */", __func__);

        r = hashmap_remove_value(NULL, "key 1", val1);
        assert_se(r == NULL);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        r = hashmap_remove_value(m, "key 1", val1);
        assert_se(r == NULL);

        hashmap_put(m, "key 1", val1);
        hashmap_put(m, "key 2", val2);

        r = hashmap_remove_value(m, "key 1", val1);
        assert_se(streq(r, "val 1"));

        r = hashmap_get(m, "key 2");
        assert_se(streq(r, "val 2"));
        assert_se(!hashmap_get(m, "key 1"));

        r = hashmap_remove_value(m, "key 2", val1);
        assert_se(r == NULL);

        r = hashmap_get(m, "key 2");
        assert_se(streq(r, "val 2"));
        assert_se(!hashmap_get(m, "key 1"));
}

static void test_hashmap_remove_and_put(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;
        int valid;
        char *r;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        valid = hashmap_remove_and_put(m, "invalid key", "new key", NULL);
        assert_se(valid == -ENOENT);

        valid = hashmap_put(m, "key 1", (void*) (const char *) "val 1");
        assert_se(valid == 1);

        valid = hashmap_remove_and_put(NULL, "key 1", "key 2", (void*) (const char *) "val 2");
        assert_se(valid == -ENOENT);

        valid = hashmap_remove_and_put(m, "key 1", "key 2", (void*) (const char *) "val 2");
        assert_se(valid == 0);

        r = hashmap_get(m, "key 2");
        assert_se(streq(r, "val 2"));
        assert_se(!hashmap_get(m, "key 1"));

        valid = hashmap_put(m, "key 3", (void*) (const char *) "val 3");
        assert_se(valid == 1);
        valid = hashmap_remove_and_put(m, "key 3", "key 2", (void*) (const char *) "val 2");
        assert_se(valid == -EEXIST);
}

static void test_hashmap_remove_and_replace(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;
        int valid;
        void *key1 = UINT_TO_PTR(1);
        void *key2 = UINT_TO_PTR(2);
        void *key3 = UINT_TO_PTR(3);
        void *r;
        int i, j;

        log_info("/* %s */", __func__);

        m = hashmap_new(&trivial_hash_ops);
        assert_se(m);

        valid = hashmap_remove_and_replace(m, key1, key2, NULL);
        assert_se(valid == -ENOENT);

        valid = hashmap_put(m, key1, key1);
        assert_se(valid == 1);

        valid = hashmap_remove_and_replace(NULL, key1, key2, key2);
        assert_se(valid == -ENOENT);

        valid = hashmap_remove_and_replace(m, key1, key2, key2);
        assert_se(valid == 0);

        r = hashmap_get(m, key2);
        assert_se(r == key2);
        assert_se(!hashmap_get(m, key1));

        valid = hashmap_put(m, key3, key3);
        assert_se(valid == 1);
        valid = hashmap_remove_and_replace(m, key3, key2, key2);
        assert_se(valid == 0);
        r = hashmap_get(m, key2);
        assert_se(r == key2);
        assert_se(!hashmap_get(m, key3));

        /* Repeat this test several times to increase the chance of hitting
         * the less likely case in hashmap_remove_and_replace where it
         * compensates for the backward shift. */
        for (i = 0; i < 20; i++) {
                hashmap_clear(m);

                for (j = 1; j < 7; j++)
                        hashmap_put(m, UINT_TO_PTR(10*i + j), UINT_TO_PTR(10*i + j));
                valid = hashmap_remove_and_replace(m, UINT_TO_PTR(10*i + 1),
                                                   UINT_TO_PTR(10*i + 2),
                                                   UINT_TO_PTR(10*i + 2));
                assert_se(valid == 0);
                assert_se(!hashmap_get(m, UINT_TO_PTR(10*i + 1)));
                for (j = 2; j < 7; j++) {
                        r = hashmap_get(m, UINT_TO_PTR(10*i + j));
                        assert_se(r == UINT_TO_PTR(10*i + j));
                }
        }
}

static void test_hashmap_ensure_allocated(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;
        int r;

        log_info("/* %s */", __func__);

        r = hashmap_ensure_allocated(&m, &string_hash_ops);
        assert_se(r == 1);

        r = hashmap_ensure_allocated(&m, &string_hash_ops);
        assert_se(r == 0);

        /* different hash ops shouldn't matter at this point */
        r = hashmap_ensure_allocated(&m, &trivial_hash_ops);
        assert_se(r == 0);
}

static void test_hashmap_foreach_key(void) {
        Hashmap *m;
        bool key_found[] = { false, false, false, false };
        const char *s;
        const char *key;
        static const char key_table[] =
                "key 1\0"
                "key 2\0"
                "key 3\0"
                "key 4\0";

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);

        NULSTR_FOREACH(key, key_table)
                hashmap_put(m, key, (void*) (const char*) "my dummy val");

        HASHMAP_FOREACH_KEY(s, key, m) {
                assert(s);
                if (!key_found[0] && streq(key, "key 1"))
                        key_found[0] = true;
                else if (!key_found[1] && streq(key, "key 2"))
                        key_found[1] = true;
                else if (!key_found[2] && streq(key, "key 3"))
                        key_found[2] = true;
                else if (!key_found[3] && streq(key, "fail"))
                        key_found[3] = true;
        }

        assert_se(m);
        assert_se(key_found[0] && key_found[1] && key_found[2] && !key_found[3]);

        hashmap_free(m);
}

static void test_hashmap_foreach(void) {
        Hashmap *m;
        bool value_found[] = { false, false, false, false };
        char *val1, *val2, *val3, *val4, *s;
        unsigned count;

        log_info("/* %s */", __func__);

        val1 = strdup("my val1");
        assert_se(val1);
        val2 = strdup("my val2");
        assert_se(val2);
        val3 = strdup("my val3");
        assert_se(val3);
        val4 = strdup("my val4");
        assert_se(val4);

        m = NULL;

        count = 0;
        HASHMAP_FOREACH(s, m)
                count++;
        assert_se(count == 0);

        m = hashmap_new(&string_hash_ops);

        count = 0;
        HASHMAP_FOREACH(s, m)
                count++;
        assert_se(count == 0);

        hashmap_put(m, "Key 1", val1);
        hashmap_put(m, "Key 2", val2);
        hashmap_put(m, "Key 3", val3);
        hashmap_put(m, "Key 4", val4);

        HASHMAP_FOREACH(s, m) {
                if (!value_found[0] && streq(s, val1))
                        value_found[0] = true;
                else if (!value_found[1] && streq(s, val2))
                        value_found[1] = true;
                else if (!value_found[2] && streq(s, val3))
                        value_found[2] = true;
                else if (!value_found[3] && streq(s, val4))
                        value_found[3] = true;
        }

        assert_se(m);
        assert_se(value_found[0] && value_found[1] && value_found[2] && value_found[3]);

        hashmap_free_free(m);
}

static void test_hashmap_merge(void) {
        Hashmap *m, *n;
        char *val1, *val2, *val3, *val4, *r;

        log_info("/* %s */", __func__);

        val1 = strdup("my val1");
        assert_se(val1);
        val2 = strdup("my val2");
        assert_se(val2);
        val3 = strdup("my val3");
        assert_se(val3);
        val4 = strdup("my val4");
        assert_se(val4);

        m = hashmap_new(&string_hash_ops);
        n = hashmap_new(&string_hash_ops);

        hashmap_put(m, "Key 1", val1);
        hashmap_put(m, "Key 2", val2);
        hashmap_put(n, "Key 3", val3);
        hashmap_put(n, "Key 4", val4);

        assert_se(hashmap_merge(m, n) == 0);
        r = hashmap_get(m, "Key 3");
        assert_se(r && streq(r, "my val3"));
        r = hashmap_get(m, "Key 4");
        assert_se(r && streq(r, "my val4"));

        assert_se(m);
        assert_se(n);
        hashmap_free(n);
        hashmap_free_free(m);
}

static void test_hashmap_contains(void) {
        Hashmap *m;
        char *val1;

        log_info("/* %s */", __func__);

        val1 = strdup("my val");
        assert_se(val1);

        m = hashmap_new(&string_hash_ops);

        assert_se(!hashmap_contains(m, "Key 1"));
        hashmap_put(m, "Key 1", val1);
        assert_se(hashmap_contains(m, "Key 1"));
        assert_se(!hashmap_contains(m, "Key 2"));

        assert_se(!hashmap_contains(NULL, "Key 1"));

        assert_se(m);
        hashmap_free_free(m);
}

static void test_hashmap_isempty(void) {
        Hashmap *m;
        char *val1;

        log_info("/* %s */", __func__);

        val1 = strdup("my val");
        assert_se(val1);

        m = hashmap_new(&string_hash_ops);

        assert_se(hashmap_isempty(m));
        hashmap_put(m, "Key 1", val1);
        assert_se(!hashmap_isempty(m));

        assert_se(m);
        hashmap_free_free(m);
}

static void test_hashmap_size(void) {
        Hashmap *m;
        char *val1, *val2, *val3, *val4;

        log_info("/* %s */", __func__);

        val1 = strdup("my val");
        assert_se(val1);
        val2 = strdup("my val");
        assert_se(val2);
        val3 = strdup("my val");
        assert_se(val3);
        val4 = strdup("my val");
        assert_se(val4);

        assert_se(hashmap_size(NULL) == 0);
        assert_se(hashmap_buckets(NULL) == 0);

        m = hashmap_new(&string_hash_ops);

        hashmap_put(m, "Key 1", val1);
        hashmap_put(m, "Key 2", val2);
        hashmap_put(m, "Key 3", val3);
        hashmap_put(m, "Key 4", val4);

        assert_se(m);
        assert_se(hashmap_size(m) == 4);
        assert_se(hashmap_buckets(m) >= 4);
        hashmap_free_free(m);
}

static void test_hashmap_get(void) {
        Hashmap *m;
        char *r;
        char *val;

        log_info("/* %s */", __func__);

        val = strdup("my val");
        assert_se(val);

        r = hashmap_get(NULL, "Key 1");
        assert_se(r == NULL);

        m = hashmap_new(&string_hash_ops);

        hashmap_put(m, "Key 1", val);

        r = hashmap_get(m, "Key 1");
        assert_se(streq(r, val));

        r = hashmap_get(m, "no such key");
        assert_se(r == NULL);

        assert_se(m);
        hashmap_free_free(m);
}

static void test_hashmap_get2(void) {
        Hashmap *m;
        char *r;
        char *val;
        char key_orig[] = "Key 1";
        void *key_copy;

        log_info("/* %s */", __func__);

        val = strdup("my val");
        assert_se(val);

        key_copy = strdup(key_orig);
        assert_se(key_copy);

        r = hashmap_get2(NULL, key_orig, &key_copy);
        assert_se(r == NULL);

        m = hashmap_new(&string_hash_ops);

        hashmap_put(m, key_copy, val);
        key_copy = NULL;

        r = hashmap_get2(m, key_orig, &key_copy);
        assert_se(streq(r, val));
        assert_se(key_orig != key_copy);
        assert_se(streq(key_orig, key_copy));

        r = hashmap_get2(m, "no such key", NULL);
        assert_se(r == NULL);

        assert_se(m);
        hashmap_free_free_free(m);
}

static void crippled_hashmap_func(const void *p, struct siphash *state) {
        return trivial_hash_func(INT_TO_PTR(PTR_TO_INT(p) & 0xff), state);
}

static const struct hash_ops crippled_hashmap_ops = {
        .hash = crippled_hashmap_func,
        .compare = trivial_compare_func,
};

static void test_hashmap_many(void) {
        Hashmap *h;
        unsigned i, j;
        void *v, *k;
        bool slow = slow_tests_enabled();
        const struct {
                const char *title;
                const struct hash_ops *ops;
                unsigned n_entries;
        } tests[] = {
                { "trivial_hashmap_ops",  NULL,                  slow ? 1 << 20 : 240 },
                { "crippled_hashmap_ops", &crippled_hashmap_ops, slow ? 1 << 14 : 140 },
        };

        log_info("/* %s (%s) */", __func__, slow ? "slow" : "fast");

        for (j = 0; j < ELEMENTSOF(tests); j++) {
                usec_t ts = now(CLOCK_MONOTONIC), n;
                char b[FORMAT_TIMESPAN_MAX];

                assert_se(h = hashmap_new(tests[j].ops));

                for (i = 1; i < tests[j].n_entries*3; i+=3) {
                        assert_se(hashmap_put(h, UINT_TO_PTR(i), UINT_TO_PTR(i)) >= 0);
                        assert_se(PTR_TO_UINT(hashmap_get(h, UINT_TO_PTR(i))) == i);
                }

                for (i = 1; i < tests[j].n_entries*3; i++)
                        assert_se(hashmap_contains(h, UINT_TO_PTR(i)) == (i % 3 == 1));

                log_info("%s %u <= %u * 0.8 = %g",
                         tests[j].title, hashmap_size(h), hashmap_buckets(h), hashmap_buckets(h) * 0.8);

                assert_se(hashmap_size(h) <= hashmap_buckets(h) * 0.8);
                assert_se(hashmap_size(h) == tests[j].n_entries);

                while (!hashmap_isempty(h)) {
                        k = hashmap_first_key(h);
                        v = hashmap_remove(h, k);
                        assert_se(v == k);
                }

                hashmap_free(h);

                n = now(CLOCK_MONOTONIC);
                log_info("test took %s", format_timespan(b, sizeof b, n - ts, 0));
        }
}

extern unsigned custom_counter;
extern const struct hash_ops boring_hash_ops, custom_hash_ops;

static void test_hashmap_free(void) {
        Hashmap *h;
        bool slow = slow_tests_enabled();
        usec_t ts, n;
        char b[FORMAT_TIMESPAN_MAX];
        unsigned n_entries = slow ? 1 << 20 : 240;

        const struct {
                const char *title;
                const struct hash_ops *ops;
                unsigned expect_counter;
        } tests[] = {
                { "string_hash_ops",      &boring_hash_ops, 2 * n_entries},
                { "custom_free_hash_ops", &custom_hash_ops, 0 },
        };

        log_info("/* %s (%s, %u entries) */", __func__, slow ? "slow" : "fast", n_entries);

        for (unsigned j = 0; j < ELEMENTSOF(tests); j++) {
                ts = now(CLOCK_MONOTONIC);
                assert_se(h = hashmap_new(tests[j].ops));

                custom_counter = 0;
                for (unsigned i = 0; i < n_entries; i++) {
                        char s[DECIMAL_STR_MAX(unsigned)];
                        char *k, *v;

                        xsprintf(s, "%u", i);
                        assert_se(k = strdup(s));
                        assert_se(v = strdup(s));
                        custom_counter += 2;

                        assert_se(hashmap_put(h, k, v) >= 0);
                }

                hashmap_free(h);

                n = now(CLOCK_MONOTONIC);
                log_info("%s test took %s", tests[j].title, format_timespan(b, sizeof b, n - ts, 0));

                assert_se(custom_counter == tests[j].expect_counter);
        }
}

typedef struct Item {
        int seen;
} Item;
static void item_seen(Item *item) {
        item->seen++;
}

static void test_hashmap_free_with_destructor(void) {
        Hashmap *m;
        struct Item items[4] = {};
        unsigned i;

        log_info("/* %s */", __func__);

        assert_se(m = hashmap_new(NULL));
        for (i = 0; i < ELEMENTSOF(items) - 1; i++)
                assert_se(hashmap_put(m, INT_TO_PTR(i), items + i) == 1);

        m = hashmap_free_with_destructor(m, item_seen);
        assert_se(items[0].seen == 1);
        assert_se(items[1].seen == 1);
        assert_se(items[2].seen == 1);
        assert_se(items[3].seen == 0);
}

static void test_hashmap_first(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        assert_se(!hashmap_first(m));
        assert_se(hashmap_put(m, "key 1", (void*) "val 1") == 1);
        assert_se(streq(hashmap_first(m), "val 1"));
        assert_se(hashmap_put(m, "key 2", (void*) "val 2") == 1);
#ifdef ORDERED
        assert_se(streq(hashmap_first(m), "val 1"));
        assert_se(hashmap_remove(m, "key 1"));
        assert_se(streq(hashmap_first(m), "val 2"));
#endif
}

static void test_hashmap_first_key(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        assert_se(!hashmap_first_key(m));
        assert_se(hashmap_put(m, "key 1", NULL) == 1);
        assert_se(streq(hashmap_first_key(m), "key 1"));
        assert_se(hashmap_put(m, "key 2", NULL) == 1);
#ifdef ORDERED
        assert_se(streq(hashmap_first_key(m), "key 1"));
        assert_se(hashmap_remove(m, "key 1") == NULL);
        assert_se(streq(hashmap_first_key(m), "key 2"));
#endif
}

static void test_hashmap_steal_first_key(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        assert_se(!hashmap_steal_first_key(m));
        assert_se(hashmap_put(m, "key 1", NULL) == 1);
        assert_se(streq(hashmap_steal_first_key(m), "key 1"));

        assert_se(hashmap_isempty(m));
}

static void test_hashmap_steal_first(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;
        int seen[3] = {};
        char *val;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        assert_se(hashmap_put(m, "key 1", (void*) "1") == 1);
        assert_se(hashmap_put(m, "key 2", (void*) "22") == 1);
        assert_se(hashmap_put(m, "key 3", (void*) "333") == 1);

        while ((val = hashmap_steal_first(m)))
                seen[strlen(val) - 1]++;

        assert_se(seen[0] == 1 && seen[1] == 1 && seen[2] == 1);

        assert_se(hashmap_isempty(m));
}

static void test_hashmap_clear_free_free(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);
        assert_se(m);

        assert_se(hashmap_put(m, strdup("key 1"), NULL) == 1);
        assert_se(hashmap_put(m, strdup("key 2"), NULL) == 1);
        assert_se(hashmap_put(m, strdup("key 3"), NULL) == 1);

        hashmap_clear_free_free(m);
        assert_se(hashmap_isempty(m));

        assert_se(hashmap_put(m, strdup("key 1"), strdup("value 1")) == 1);
        assert_se(hashmap_put(m, strdup("key 2"), strdup("value 2")) == 1);
        assert_se(hashmap_put(m, strdup("key 3"), strdup("value 3")) == 1);

        hashmap_clear_free_free(m);
        assert_se(hashmap_isempty(m));
}

DEFINE_PRIVATE_HASH_OPS_WITH_KEY_DESTRUCTOR(test_hash_ops_key, char, string_hash_func, string_compare_func, free);
DEFINE_PRIVATE_HASH_OPS_FULL(test_hash_ops_full, char, string_hash_func, string_compare_func, free, char, free);

static void test_hashmap_clear_free_with_destructor(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;

        log_info("/* %s */", __func__);

        m = hashmap_new(&test_hash_ops_key);
        assert_se(m);

        assert_se(hashmap_put(m, strdup("key 1"), NULL) == 1);
        assert_se(hashmap_put(m, strdup("key 2"), NULL) == 1);
        assert_se(hashmap_put(m, strdup("key 3"), NULL) == 1);

        hashmap_clear_free(m);
        assert_se(hashmap_isempty(m));
        m = hashmap_free(m);

        m = hashmap_new(&test_hash_ops_full);
        assert_se(m);

        assert_se(hashmap_put(m, strdup("key 1"), strdup("value 1")) == 1);
        assert_se(hashmap_put(m, strdup("key 2"), strdup("value 2")) == 1);
        assert_se(hashmap_put(m, strdup("key 3"), strdup("value 3")) == 1);

        hashmap_clear_free(m);
        assert_se(hashmap_isempty(m));
}

static void test_hashmap_reserve(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;

        log_info("/* %s */", __func__);

        m = hashmap_new(&string_hash_ops);

        assert_se(hashmap_reserve(m, 1) == 0);
        assert_se(hashmap_buckets(m) < 1000);
        assert_se(hashmap_reserve(m, 1000) == 0);
        assert_se(hashmap_buckets(m) >= 1000);
        assert_se(hashmap_isempty(m));

        assert_se(hashmap_put(m, "key 1", (void*) "val 1") == 1);

        assert_se(hashmap_reserve(m, UINT_MAX) == -ENOMEM);
        assert_se(hashmap_reserve(m, UINT_MAX - 1) == -ENOMEM);
}

static void test_path_hashmap(void) {
        _cleanup_hashmap_free_ Hashmap *h = NULL;

        log_info("/* %s */", __func__);

        assert_se(h = hashmap_new(&path_hash_ops));

        assert_se(hashmap_put(h, "foo", INT_TO_PTR(1)) >= 0);
        assert_se(hashmap_put(h, "/foo", INT_TO_PTR(2)) >= 0);
        assert_se(hashmap_put(h, "//foo", INT_TO_PTR(3)) == -EEXIST);
        assert_se(hashmap_put(h, "//foox/", INT_TO_PTR(4)) >= 0);
        assert_se(hashmap_put(h, "/foox////", INT_TO_PTR(5)) == -EEXIST);
        assert_se(hashmap_put(h, "foo//////bar/quux//", INT_TO_PTR(6)) >= 0);
        assert_se(hashmap_put(h, "foo/bar//quux/", INT_TO_PTR(8)) == -EEXIST);

        assert_se(hashmap_get(h, "foo") == INT_TO_PTR(1));
        assert_se(hashmap_get(h, "foo/") == INT_TO_PTR(1));
        assert_se(hashmap_get(h, "foo////") == INT_TO_PTR(1));
        assert_se(hashmap_get(h, "/foo") == INT_TO_PTR(2));
        assert_se(hashmap_get(h, "//foo") == INT_TO_PTR(2));
        assert_se(hashmap_get(h, "/////foo////") == INT_TO_PTR(2));
        assert_se(hashmap_get(h, "/////foox////") == INT_TO_PTR(4));
        assert_se(hashmap_get(h, "/foox/") == INT_TO_PTR(4));
        assert_se(hashmap_get(h, "/foox") == INT_TO_PTR(4));
        assert_se(!hashmap_get(h, "foox"));
        assert_se(hashmap_get(h, "foo/bar/quux") == INT_TO_PTR(6));
        assert_se(hashmap_get(h, "foo////bar////quux/////") == INT_TO_PTR(6));
        assert_se(!hashmap_get(h, "/foo////bar////quux/////"));
}

#if 0 /// UNNEEDED by elogind
static void test_string_strv_hashmap(void) {
        _cleanup_hashmap_free_ Hashmap *m = NULL;
        char **s;

        log_info("/* %s */", __func__);

        assert_se(string_strv_hashmap_put(&m, "foo", "bar") == 1);
        assert_se(string_strv_hashmap_put(&m, "foo", "bar") == 0);
        assert_se(string_strv_hashmap_put(&m, "foo", "BAR") == 1);
        assert_se(string_strv_hashmap_put(&m, "foo", "BAR") == 0);
        assert_se(string_strv_hashmap_put(&m, "foo", "bar") == 0);
        assert_se(hashmap_contains(m, "foo"));

        s = hashmap_get(m, "foo");
        assert_se(strv_equal(s, STRV_MAKE("bar", "BAR")));

        assert_se(string_strv_hashmap_put(&m, "xxx", "bar") == 1);
        assert_se(string_strv_hashmap_put(&m, "xxx", "bar") == 0);
        assert_se(string_strv_hashmap_put(&m, "xxx", "BAR") == 1);
        assert_se(string_strv_hashmap_put(&m, "xxx", "BAR") == 0);
        assert_se(string_strv_hashmap_put(&m, "xxx", "bar") == 0);
        assert_se(hashmap_contains(m, "xxx"));

        s = hashmap_get(m, "xxx");
        assert_se(strv_equal(s, STRV_MAKE("bar", "BAR")));
}
#endif // 0

void test_hashmap_funcs(void) {
        log_info("/************ %s ************/", __func__);

        test_hashmap_copy();
        test_hashmap_get_strv();
        test_hashmap_move_one();
        test_hashmap_move();
        test_hashmap_replace();
        test_hashmap_update();
        test_hashmap_put();
        test_hashmap_remove();
        test_hashmap_remove2();
        test_hashmap_remove_value();
        test_hashmap_remove_and_put();
        test_hashmap_remove_and_replace();
        test_hashmap_ensure_allocated();
        test_hashmap_foreach();
        test_hashmap_foreach_key();
        test_hashmap_contains();
        test_hashmap_merge();
        test_hashmap_isempty();
        test_hashmap_get();
        test_hashmap_get2();
        test_hashmap_size();
        test_hashmap_many();
        test_hashmap_free();
        test_hashmap_free_with_destructor();
        test_hashmap_first();
        test_hashmap_first_key();
        test_hashmap_steal_first_key();
        test_hashmap_steal_first();
        test_hashmap_clear_free_free();
        test_hashmap_clear_free_with_destructor();
        test_hashmap_reserve();
        test_path_hashmap();
#if 0 /// UNNEEDED by elogind
        test_string_strv_hashmap();
#endif // 0
}
