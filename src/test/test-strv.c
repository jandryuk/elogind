/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "alloc-util.h"
#include "escape.h"
#include "nulstr-util.h"
//#include "specifier.h"
#include "string-util.h"
#include "strv.h"

#if 0 /// UNNEEDED by elogind
static void test_specifier_printf(void) {
        static const Specifier table[] = {
                { 'X', specifier_string,         (char*) "AAAA" },
                { 'Y', specifier_string,         (char*) "BBBB" },
                COMMON_SYSTEM_SPECIFIERS,
                {}
        };

        _cleanup_free_ char *w = NULL;
        int r;

        log_info("/* %s */", __func__);

        r = specifier_printf("xxx a=%X b=%Y yyy", table, NULL, &w);
        assert_se(r >= 0);
        assert_se(w);

        puts(w);
        assert_se(streq(w, "xxx a=AAAA b=BBBB yyy"));

        free(w);
        r = specifier_printf("machine=%m, boot=%b, host=%H, version=%v, arch=%a", table, NULL, &w);
        assert_se(r >= 0);
        assert_se(w);
        puts(w);

        w = mfree(w);
        specifier_printf("os=%o, os-version=%w, build=%B, variant=%W", table, NULL, &w);
        if (w)
                puts(w);
}
#endif // 0

static void test_str_in_set(void) {
        log_info("/* %s */", __func__);

        assert_se(STR_IN_SET("x", "x", "y", "z"));
        assert_se(!STR_IN_SET("X", "x", "y", "z"));
        assert_se(!STR_IN_SET("", "x", "y", "z"));
        assert_se(STR_IN_SET("x", "w", "x"));
}

static void test_strptr_in_set(void) {
        log_info("/* %s */", __func__);

        assert_se(STRPTR_IN_SET("x", "x", "y", "z"));
        assert_se(!STRPTR_IN_SET("X", "x", "y", "z"));
        assert_se(!STRPTR_IN_SET("", "x", "y", "z"));
        assert_se(STRPTR_IN_SET("x", "w", "x"));

        assert_se(!STRPTR_IN_SET(NULL, "x", "y", "z"));
        assert_se(!STRPTR_IN_SET(NULL, ""));
        /* strv cannot contain a null, hence the result below */
        assert_se(!STRPTR_IN_SET(NULL, NULL));
}

static void test_startswith_set(void) {
        log_info("/* %s */", __func__);

        assert_se(!STARTSWITH_SET("foo", "bar", "baz", "waldo"));
        assert_se(!STARTSWITH_SET("foo", "bar"));

        assert_se(STARTSWITH_SET("abc", "a", "ab", "abc"));
        assert_se(STARTSWITH_SET("abc", "ax", "ab", "abc"));
        assert_se(STARTSWITH_SET("abc", "ax", "abx", "abc"));
        assert_se(!STARTSWITH_SET("abc", "ax", "abx", "abcx"));

        assert_se(streq_ptr(STARTSWITH_SET("foobar", "hhh", "kkk", "foo", "zzz"), "bar"));
        assert_se(streq_ptr(STARTSWITH_SET("foobar", "hhh", "kkk", "", "zzz"), "foobar"));
        assert_se(streq_ptr(STARTSWITH_SET("", "hhh", "kkk", "zzz", ""), ""));
}

static const char* const input_table_multiple[] = {
        "one",
        "two",
        "three",
        NULL,
};

static const char* const input_table_quoted[] = {
        "one",
        "  two\t three ",
        " four  five",
        NULL,
};

static const char* const input_table_quoted_joined[] = {
        "one",
        "  two\t three " " four  five",
        NULL,
};

static const char* const input_table_one[] = {
        "one",
        NULL,
};

static const char* const input_table_none[] = {
        NULL,
};

static const char* const input_table_two_empties[] = {
        "",
        "",
        NULL,
};

static const char* const input_table_one_empty[] = {
        "",
        NULL,
};

#if 0 /// UNNEEDED by elogind
static const char* const input_table_unescape[] = {
        "ID_VENDOR=QEMU",
        "ID_VENDOR_ENC=QEMUx20x20x20x20",
        "ID_MODEL_ENC=QEMUx20HARDDISKx20x20x20",
        NULL,
};

static const char* const input_table_retain_escape[] = {
        "ID_VENDOR=QEMU",
        "ID_VENDOR_ENC=QEMU\\x20\\x20\\x20\\x20",
        "ID_MODEL_ENC=QEMU\\x20HARDDISK\\x20\\x20\\x20",
        NULL,
};
#endif // 0

static void test_strv_find(void) {
        log_info("/* %s */", __func__);

        assert_se(strv_find((char **)input_table_multiple, "three"));
        assert_se(!strv_find((char **)input_table_multiple, "four"));
}

static void test_strv_find_prefix(void) {
        log_info("/* %s */", __func__);

        assert_se(strv_find_prefix((char **)input_table_multiple, "o"));
        assert_se(strv_find_prefix((char **)input_table_multiple, "one"));
        assert_se(strv_find_prefix((char **)input_table_multiple, ""));
        assert_se(!strv_find_prefix((char **)input_table_multiple, "xxx"));
        assert_se(!strv_find_prefix((char **)input_table_multiple, "onee"));
}

static void test_strv_find_startswith(void) {
        char *r;

        log_info("/* %s */", __func__);

        r = strv_find_startswith((char **)input_table_multiple, "o");
        assert_se(r && streq(r, "ne"));

        r = strv_find_startswith((char **)input_table_multiple, "one");
        assert_se(r && streq(r, ""));

        r = strv_find_startswith((char **)input_table_multiple, "");
        assert_se(r && streq(r, "one"));

        assert_se(!strv_find_startswith((char **)input_table_multiple, "xxx"));
        assert_se(!strv_find_startswith((char **)input_table_multiple, "onee"));
}

static void test_strv_join(void) {
        log_info("/* %s */", __func__);

        _cleanup_free_ char *p = strv_join((char **)input_table_multiple, ", ");
        assert_se(p);
        assert_se(streq(p, "one, two, three"));

        _cleanup_free_ char *q = strv_join((char **)input_table_multiple, ";");
        assert_se(q);
        assert_se(streq(q, "one;two;three"));

        _cleanup_free_ char *r = strv_join((char **)input_table_multiple, NULL);
        assert_se(r);
        assert_se(streq(r, "one two three"));

        _cleanup_free_ char *s = strv_join(STRV_MAKE("1", "2", "3,3"), ",");
        assert_se(s);
        assert_se(streq(s, "1,2,3,3"));

        _cleanup_free_ char *t = strv_join((char **)input_table_one, ", ");
        assert_se(t);
        assert_se(streq(t, "one"));

        _cleanup_free_ char *u = strv_join((char **)input_table_none, ", ");
        assert_se(u);
        assert_se(streq(u, ""));

        _cleanup_free_ char *v = strv_join((char **)input_table_two_empties, ", ");
        assert_se(v);
        assert_se(streq(v, ", "));

        _cleanup_free_ char *w = strv_join((char **)input_table_one_empty, ", ");
        assert_se(w);
        assert_se(streq(w, ""));
}

static void test_strv_join_full(void) {
        log_info("/* %s */", __func__);

        _cleanup_free_ char *p = strv_join_full((char **)input_table_multiple, ", ", "foo", false);
        assert_se(p);
        assert_se(streq(p, "fooone, footwo, foothree"));

        _cleanup_free_ char *q = strv_join_full((char **)input_table_multiple, ";", "foo", false);
        assert_se(q);
        assert_se(streq(q, "fooone;footwo;foothree"));

        _cleanup_free_ char *r = strv_join_full(STRV_MAKE("a", "a;b", "a:c"), ";", NULL, true);
        assert_se(r);
        assert_se(streq(r, "a;a\\;b;a:c"));

        _cleanup_free_ char *s = strv_join_full(STRV_MAKE("a", "a;b", "a;;c", ";", ";x"), ";", NULL, true);
        assert_se(s);
        assert_se(streq(s, "a;a\\;b;a\\;\\;c;\\;;\\;x"));

        _cleanup_free_ char *t = strv_join_full(STRV_MAKE("a", "a;b", "a:c", ";"), ";", "=", true);
        assert_se(t);
        assert_se(streq(t, "=a;=a\\;b;=a:c;=\\;"));
        t = mfree(t);

        _cleanup_free_ char *u = strv_join_full((char **)input_table_multiple, NULL, "foo", false);
        assert_se(u);
        assert_se(streq(u, "fooone footwo foothree"));

        _cleanup_free_ char *v = strv_join_full((char **)input_table_one, ", ", "foo", false);
        assert_se(v);
        assert_se(streq(v, "fooone"));

        _cleanup_free_ char *w = strv_join_full((char **)input_table_none, ", ", "foo", false);
        assert_se(w);
        assert_se(streq(w, ""));

        _cleanup_free_ char *x = strv_join_full((char **)input_table_two_empties, ", ", "foo", false);
        assert_se(x);
        assert_se(streq(x, "foo, foo"));

        _cleanup_free_ char *y = strv_join_full((char **)input_table_one_empty, ", ", "foo", false);
        assert_se(y);
        assert_se(streq(y, "foo"));
}

static void test_strv_unquote(const char *quoted, char **list) {
        _cleanup_strv_free_ char **s;
        _cleanup_free_ char *j;
        unsigned i = 0;
        char **t;
        int r;

        log_info("/* %s */", __func__);

        r = strv_split_full(&s, quoted, WHITESPACE, EXTRACT_UNQUOTE);
        assert_se(r == (int) strv_length(list));
        assert_se(s);
        j = strv_join(s, " | ");
        assert_se(j);
        puts(j);

        STRV_FOREACH(t, s)
                assert_se(streq(list[i++], *t));

        assert_se(list[i] == NULL);
}

static void test_invalid_unquote(const char *quoted) {
        char **s = NULL;
        int r;

        log_info("/* %s */", __func__);

        r = strv_split_full(&s, quoted, WHITESPACE, EXTRACT_UNQUOTE);
        assert_se(s == NULL);
        assert_se(r == -EINVAL);
}

static void test_strv_split(void) {
        _cleanup_(strv_free_erasep) char **l = NULL;
        const char str[] = "one,two,three";

        log_info("/* %s */", __func__);

        l = strv_split(str, ",");
        assert_se(l);
        assert_se(strv_equal(l, (char**) input_table_multiple));

        strv_free_erase(l);

        l = strv_split("    one    two\t three", WHITESPACE);
        assert_se(l);
        assert_se(strv_equal(l, (char**) input_table_multiple));

        strv_free_erase(l);

        /* Setting NULL for separator is equivalent to WHITESPACE */
        l = strv_split("    one    two\t three", NULL);
        assert_se(l);
        assert_se(strv_equal(l, (char**) input_table_multiple));

        strv_free_erase(l);

        assert_se(strv_split_full(&l, "    one    two\t three", NULL, 0) == 3);
        assert_se(strv_equal(l, (char**) input_table_multiple));

        strv_free_erase(l);

        assert_se(strv_split_full(&l, "    'one'  \"  two\t three \" ' four  five'", NULL, EXTRACT_UNQUOTE) == 3);
        assert_se(strv_equal(l, (char**) input_table_quoted));

        l = strv_free_erase(l);

        /* missing last quote causes extraction to fail. */
        assert_se(strv_split_full(&l, "    'one'  \"  two\t three \" ' four  five", NULL, EXTRACT_UNQUOTE) == -EINVAL);
        assert_se(!l);

        /* missing last quote, but the last element is _not_ ignored with EXTRACT_RELAX. */
        assert_se(strv_split_full(&l, "    'one'  \"  two\t three \" ' four  five", NULL, EXTRACT_UNQUOTE | EXTRACT_RELAX) == 3);
        assert_se(strv_equal(l, (char**) input_table_quoted));

        l = strv_free_erase(l);

        /* missing separator between items */
        assert_se(strv_split_full(&l, "    'one'  \"  two\t three \"' four  five'", NULL, EXTRACT_UNQUOTE | EXTRACT_RELAX) == 2);
        assert_se(strv_equal(l, (char**) input_table_quoted_joined));

        l = strv_free_erase(l);

        assert_se(strv_split_full(&l, "    'one'  \"  two\t three \"' four  five", NULL,
                                     EXTRACT_UNQUOTE | EXTRACT_RELAX | EXTRACT_UNESCAPE_RELAX) == 2);
        assert_se(strv_equal(l, (char**) input_table_quoted_joined));

        l = strv_free_erase(l);

        assert_se(strv_split_full(&l, "\\", NULL, EXTRACT_UNQUOTE | EXTRACT_RELAX | EXTRACT_UNESCAPE_RELAX) == 1);
        assert_se(strv_equal(l, STRV_MAKE("\\")));
}

static void test_strv_split_empty(void) {
        _cleanup_strv_free_ char **l = NULL;

        log_info("/* %s */", __func__);

        l = strv_split("", WHITESPACE);
        assert_se(l);
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(l = strv_split("", NULL));
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(strv_split_full(&l, "", NULL, 0) == 0);
        assert_se(l);
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(strv_split_full(&l, "", NULL, EXTRACT_UNQUOTE) == 0);
        assert_se(l);
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(strv_split_full(&l, "", WHITESPACE, EXTRACT_UNQUOTE) == 0);
        assert_se(l);
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(strv_split_full(&l, "", WHITESPACE, EXTRACT_UNQUOTE | EXTRACT_RELAX) == 0);
        assert_se(l);
        assert_se(strv_isempty(l));
        strv_free(l);

        l = strv_split("    ", WHITESPACE);
        assert_se(l);
        assert_se(strv_isempty(l));
        strv_free(l);

        l = strv_split("    ", NULL);
        assert_se(l);
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(strv_split_full(&l, "    ", NULL, 0) == 0);
        assert_se(l);
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(strv_split_full(&l, "    ", WHITESPACE, EXTRACT_UNQUOTE) == 0);
        assert_se(l);
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(strv_split_full(&l, "    ", NULL, EXTRACT_UNQUOTE) == 0);
        assert_se(l);
        assert_se(strv_isempty(l));
        l = strv_free(l);

        assert_se(strv_split_full(&l, "    ", NULL, EXTRACT_UNQUOTE | EXTRACT_RELAX) == 0);
        assert_se(l);
        assert_se(strv_isempty(l));
}

static void test_strv_split_full(void) {
        _cleanup_strv_free_ char **l = NULL;
        const char *str = ":foo\\:bar::waldo:";
        int r;

        log_info("/* %s */", __func__);

        r = strv_split_full(&l, str, ":", EXTRACT_DONT_COALESCE_SEPARATORS);
        assert_se(r == (int) strv_length(l));
        assert_se(streq_ptr(l[0], ""));
        assert_se(streq_ptr(l[1], "foo:bar"));
        assert_se(streq_ptr(l[2], ""));
        assert_se(streq_ptr(l[3], "waldo"));
        assert_se(streq_ptr(l[4], ""));
        assert_se(streq_ptr(l[5], NULL));
}

static void test_strv_split_colon_pairs(void) {
        _cleanup_strv_free_ char **l = NULL;
        const char *str = "one:two three four:five six seven:eight\\:nine ten\\:eleven\\\\",
                   *str_inval="one:two three:four:five";
        int r;

        log_info("/* %s */", __func__);

        r = strv_split_colon_pairs(&l, str);
        assert_se(r == (int) strv_length(l));
        assert_se(r == 12);
        assert_se(streq_ptr(l[0], "one"));
        assert_se(streq_ptr(l[1], "two"));
        assert_se(streq_ptr(l[2], "three"));
        assert_se(streq_ptr(l[3], ""));
        assert_se(streq_ptr(l[4], "four"));
        assert_se(streq_ptr(l[5], "five"));
        assert_se(streq_ptr(l[6], "six"));
        assert_se(streq_ptr(l[7], ""));
        assert_se(streq_ptr(l[8], "seven"));
        assert_se(streq_ptr(l[9], "eight:nine"));
        assert_se(streq_ptr(l[10], "ten:eleven\\"));
        assert_se(streq_ptr(l[11], ""));
        assert_se(streq_ptr(l[12], NULL));

        r = strv_split_colon_pairs(&l, str_inval);
        assert_se(r == -EINVAL);
}

#if 0 /// UNNEEDED by elogind
static void test_strv_split_newlines(void) {
        unsigned i = 0;
        char **s;
        _cleanup_strv_free_ char **l = NULL;
        const char str[] = "one\ntwo\nthree";

        log_info("/* %s */", __func__);

        l = strv_split_newlines(str);
        assert_se(l);

        STRV_FOREACH(s, l)
                assert_se(streq(*s, input_table_multiple[i++]));
}

static void test_strv_split_newlines_full(void) {
        const char str[] =
                "ID_VENDOR=QEMU\n"
                "ID_VENDOR_ENC=QEMU\\x20\\x20\\x20\\x20\n"
                "ID_MODEL_ENC=QEMU\\x20HARDDISK\\x20\\x20\\x20\n"
                "\n\n\n";
        _cleanup_strv_free_ char **l = NULL;

        log_info("/* %s */", __func__);

        assert_se(strv_split_newlines_full(&l, str, 0) == 3);
        assert_se(strv_equal(l, (char**) input_table_unescape));

        l = strv_free(l);

        assert_se(strv_split_newlines_full(&l, str, EXTRACT_RETAIN_ESCAPE) == 3);
        assert_se(strv_equal(l, (char**) input_table_retain_escape));
}
#endif // 0

static void test_strv_split_nulstr(void) {
        _cleanup_strv_free_ char **l = NULL;
        const char nulstr[] = "str0\0str1\0str2\0str3\0";

        log_info("/* %s */", __func__);

        l = strv_split_nulstr (nulstr);
        assert_se(l);

        assert_se(streq(l[0], "str0"));
        assert_se(streq(l[1], "str1"));
        assert_se(streq(l[2], "str2"));
        assert_se(streq(l[3], "str3"));
}

static void test_strv_parse_nulstr(void) {
        _cleanup_strv_free_ char **l = NULL;
        const char nulstr[] = "hoge\0hoge2\0hoge3\0\0hoge5\0\0xxx";

        log_info("/* %s */", __func__);

        l = strv_parse_nulstr(nulstr, sizeof(nulstr)-1);
        assert_se(l);
        puts("Parse nulstr:");
        strv_print(l);

        assert_se(streq(l[0], "hoge"));
        assert_se(streq(l[1], "hoge2"));
        assert_se(streq(l[2], "hoge3"));
        assert_se(streq(l[3], ""));
        assert_se(streq(l[4], "hoge5"));
        assert_se(streq(l[5], ""));
        assert_se(streq(l[6], "xxx"));
}

#if 0 /// UNNEEDED by elogind
static void test_strv_overlap(void) {
        const char * const input_table[] = {
                "one",
                "two",
                "three",
                NULL
        };
        const char * const input_table_overlap[] = {
                "two",
                NULL
        };
        const char * const input_table_unique[] = {
                "four",
                "five",
                "six",
                NULL
        };

        log_info("/* %s */", __func__);

        assert_se(strv_overlap((char **)input_table, (char**)input_table_overlap));
        assert_se(!strv_overlap((char **)input_table, (char**)input_table_unique));
}
#endif // 0

static void test_strv_sort(void) {
        const char* input_table[] = {
                "durian",
                "apple",
                "citrus",
                 "CAPITAL LETTERS FIRST",
                "banana",
                NULL
        };

        log_info("/* %s */", __func__);

        strv_sort((char **)input_table);

        assert_se(streq(input_table[0], "CAPITAL LETTERS FIRST"));
        assert_se(streq(input_table[1], "apple"));
        assert_se(streq(input_table[2], "banana"));
        assert_se(streq(input_table[3], "citrus"));
        assert_se(streq(input_table[4], "durian"));
}

#if 0 /// UNNEEDED by elogind
static void test_strv_extend_strv_concat(void) {
        _cleanup_strv_free_ char **a = NULL, **b = NULL;

        log_info("/* %s */", __func__);

        a = strv_new("without", "suffix");
        b = strv_new("with", "suffix");
        assert_se(a);
        assert_se(b);

        assert_se(strv_extend_strv_concat(&a, b, "_suffix") >= 0);

        assert_se(streq(a[0], "without"));
        assert_se(streq(a[1], "suffix"));
        assert_se(streq(a[2], "with_suffix"));
        assert_se(streq(a[3], "suffix_suffix"));
}
#endif // 0

static void test_strv_extend_strv(void) {
        _cleanup_strv_free_ char **a = NULL, **b = NULL, **n = NULL;

        log_info("/* %s */", __func__);

        a = strv_new("abc", "def", "ghi");
        b = strv_new("jkl", "mno", "abc", "pqr");
        assert_se(a);
        assert_se(b);

        assert_se(strv_extend_strv(&a, b, true) == 3);

        assert_se(streq(a[0], "abc"));
        assert_se(streq(a[1], "def"));
        assert_se(streq(a[2], "ghi"));
        assert_se(streq(a[3], "jkl"));
        assert_se(streq(a[4], "mno"));
        assert_se(streq(a[5], "pqr"));
        assert_se(strv_length(a) == 6);

        assert_se(strv_extend_strv(&n, b, false) >= 0);
        assert_se(streq(n[0], "jkl"));
        assert_se(streq(n[1], "mno"));
        assert_se(streq(n[2], "abc"));
        assert_se(streq(n[3], "pqr"));
        assert_se(strv_length(n) == 4);
}

static void test_strv_extend(void) {
        _cleanup_strv_free_ char **a = NULL, **b = NULL;

        log_info("/* %s */", __func__);

        a = strv_new("test", "test1");
        assert_se(a);
        assert_se(strv_extend(&a, "test2") >= 0);
        assert_se(strv_extend(&b, "test3") >= 0);

        assert_se(streq(a[0], "test"));
        assert_se(streq(a[1], "test1"));
        assert_se(streq(a[2], "test2"));
        assert_se(streq(b[0], "test3"));
}

#if 0 /// UNNEEDED by elogind
static void test_strv_extendf(void) {
        _cleanup_strv_free_ char **a = NULL, **b = NULL;

        log_info("/* %s */", __func__);

        a = strv_new("test", "test1");
        assert_se(a);
        assert_se(strv_extendf(&a, "test2 %s %d %s", "foo", 128, "bar") >= 0);
        assert_se(strv_extendf(&b, "test3 %s %s %d", "bar", "foo", 128) >= 0);

        assert_se(streq(a[0], "test"));
        assert_se(streq(a[1], "test1"));
        assert_se(streq(a[2], "test2 foo 128 bar"));
        assert_se(streq(b[0], "test3 bar foo 128"));
}
#endif // 0

static void test_strv_foreach(void) {
        _cleanup_strv_free_ char **a;
        unsigned i = 0;
        char **check;

        log_info("/* %s */", __func__);

        a = strv_new("one", "two", "three");
        assert_se(a);

        STRV_FOREACH(check, a)
                assert_se(streq(*check, input_table_multiple[i++]));
}

static void test_strv_foreach_backwards(void) {
        _cleanup_strv_free_ char **a;
        unsigned i = 2;
        char **check;

        log_info("/* %s */", __func__);

        a = strv_new("one", "two", "three");

        assert_se(a);

        STRV_FOREACH_BACKWARDS(check, a)
                assert_se(streq_ptr(*check, input_table_multiple[i--]));

        STRV_FOREACH_BACKWARDS(check, (char**) NULL)
                assert_not_reached("Let's see that we check empty strv right, too.");

        STRV_FOREACH_BACKWARDS(check, (char**) { NULL })
                assert_not_reached("Let's see that we check empty strv right, too.");
}

static void test_strv_foreach_pair(void) {
        _cleanup_strv_free_ char **a = NULL;
        char **x, **y;

        log_info("/* %s */", __func__);

        a = strv_new("pair_one",   "pair_one",
                     "pair_two",   "pair_two",
                     "pair_three", "pair_three");
        STRV_FOREACH_PAIR(x, y, a)
                assert_se(streq(*x, *y));
}

static void test_strv_from_stdarg_alloca_one(char **l, const char *first, ...) {
        char **j;
        unsigned i;

        log_info("/* %s */", __func__);

        j = strv_from_stdarg_alloca(first);

        for (i = 0;; i++) {
                assert_se(streq_ptr(l[i], j[i]));

                if (!l[i])
                        break;
        }
}

static void test_strv_from_stdarg_alloca(void) {
        log_info("/* %s */", __func__);

        test_strv_from_stdarg_alloca_one(STRV_MAKE("foo", "bar"), "foo", "bar", NULL);
        test_strv_from_stdarg_alloca_one(STRV_MAKE("foo"), "foo", NULL);
        test_strv_from_stdarg_alloca_one(STRV_MAKE_EMPTY, NULL);
}

static void test_strv_insert(void) {
        _cleanup_strv_free_ char **a = NULL;

        log_info("/* %s */", __func__);

        assert_se(strv_insert(&a, 0, strdup("first")) == 0);
        assert_se(streq(a[0], "first"));
        assert_se(!a[1]);

        assert_se(strv_insert(&a, 0, NULL) == 0);
        assert_se(streq(a[0], "first"));
        assert_se(!a[1]);

        assert_se(strv_insert(&a, 1, strdup("two")) == 0);
        assert_se(streq(a[0], "first"));
        assert_se(streq(a[1], "two"));
        assert_se(!a[2]);

        assert_se(strv_insert(&a, 4, strdup("tri")) == 0);
        assert_se(streq(a[0], "first"));
        assert_se(streq(a[1], "two"));
        assert_se(streq(a[2], "tri"));
        assert_se(!a[3]);

        assert_se(strv_insert(&a, 1, strdup("duo")) == 0);
        assert_se(streq(a[0], "first"));
        assert_se(streq(a[1], "duo"));
        assert_se(streq(a[2], "two"));
        assert_se(streq(a[3], "tri"));
        assert_se(!a[4]);
}

static void test_strv_push_prepend(void) {
        _cleanup_strv_free_ char **a = NULL;

        log_info("/* %s */", __func__);

        assert_se(a = strv_new("foo", "bar", "three"));

        assert_se(strv_push_prepend(&a, strdup("first")) >= 0);
        assert_se(streq(a[0], "first"));
        assert_se(streq(a[1], "foo"));
        assert_se(streq(a[2], "bar"));
        assert_se(streq(a[3], "three"));
        assert_se(!a[4]);

        assert_se(strv_consume_prepend(&a, strdup("first2")) >= 0);
        assert_se(streq(a[0], "first2"));
        assert_se(streq(a[1], "first"));
        assert_se(streq(a[2], "foo"));
        assert_se(streq(a[3], "bar"));
        assert_se(streq(a[4], "three"));
        assert_se(!a[5]);
}

static void test_strv_push(void) {
        _cleanup_strv_free_ char **a = NULL;
        char *i, *j;

        log_info("/* %s */", __func__);

        assert_se(i = strdup("foo"));
        assert_se(strv_push(&a, i) >= 0);

        assert_se(i = strdup("a"));
        assert_se(j = strdup("b"));
        assert_se(strv_push_pair(&a, i, j) >= 0);

        assert_se(streq_ptr(a[0], "foo"));
        assert_se(streq_ptr(a[1], "a"));
        assert_se(streq_ptr(a[2], "b"));
        assert_se(streq_ptr(a[3], NULL));
}

static void test_strv_compare(void) {
        _cleanup_strv_free_ char **a = NULL;
        _cleanup_strv_free_ char **b = NULL;
        _cleanup_strv_free_ char **c = NULL;
        _cleanup_strv_free_ char **d = NULL;

        log_info("/* %s */", __func__);

        a = strv_new("one", "two", "three");
        assert_se(a);
        b = strv_new("one", "two", "three");
        assert_se(b);
        c = strv_new("one", "two", "three", "four");
        assert_se(c);
        d = strv_new(NULL);
        assert_se(d);

        assert_se(strv_compare(a, a) == 0);
        assert_se(strv_compare(a, b) == 0);
        assert_se(strv_compare(d, d) == 0);
        assert_se(strv_compare(d, NULL) == 0);
        assert_se(strv_compare(NULL, NULL) == 0);

        assert_se(strv_compare(a, c) < 0);
        assert_se(strv_compare(b, c) < 0);
        assert_se(strv_compare(b, d) == 1);
        assert_se(strv_compare(b, NULL) == 1);
}

#if 0 /// UNNEEDED by elogind
static void test_strv_is_uniq(void) {
        _cleanup_strv_free_ char **a = NULL, **b = NULL, **c = NULL, **d = NULL;

        log_info("/* %s */", __func__);

        a = strv_new(NULL);
        assert_se(a);
        assert_se(strv_is_uniq(a));

        b = strv_new("foo");
        assert_se(b);
        assert_se(strv_is_uniq(b));

        c = strv_new("foo", "bar");
        assert_se(c);
        assert_se(strv_is_uniq(c));

        d = strv_new("foo", "bar", "waldo", "bar", "piep");
        assert_se(d);
        assert_se(!strv_is_uniq(d));
}

static void test_strv_reverse(void) {
        _cleanup_strv_free_ char **a = NULL, **b = NULL, **c = NULL, **d = NULL;

        log_info("/* %s */", __func__);

        a = strv_new(NULL);
        assert_se(a);

        strv_reverse(a);
        assert_se(strv_isempty(a));

        b = strv_new("foo");
        assert_se(b);
        strv_reverse(b);
        assert_se(streq_ptr(b[0], "foo"));
        assert_se(streq_ptr(b[1], NULL));

        c = strv_new("foo", "bar");
        assert_se(c);
        strv_reverse(c);
        assert_se(streq_ptr(c[0], "bar"));
        assert_se(streq_ptr(c[1], "foo"));
        assert_se(streq_ptr(c[2], NULL));

        d = strv_new("foo", "bar", "waldo");
        assert_se(d);
        strv_reverse(d);
        assert_se(streq_ptr(d[0], "waldo"));
        assert_se(streq_ptr(d[1], "bar"));
        assert_se(streq_ptr(d[2], "foo"));
        assert_se(streq_ptr(d[3], NULL));
}

static void test_strv_shell_escape(void) {
        _cleanup_strv_free_ char **v = NULL;

        log_info("/* %s */", __func__);

        v = strv_new("foo:bar", "bar,baz", "wal\\do");
        assert_se(v);
        assert_se(strv_shell_escape(v, ",:"));
        assert_se(streq_ptr(v[0], "foo\\:bar"));
        assert_se(streq_ptr(v[1], "bar\\,baz"));
        assert_se(streq_ptr(v[2], "wal\\\\do"));
        assert_se(streq_ptr(v[3], NULL));
}
#endif // 0

static void test_strv_skip_one(char **a, size_t n, char **b) {
        a = strv_skip(a, n);
        assert_se(strv_equal(a, b));
}

static void test_strv_skip(void) {
        log_info("/* %s */", __func__);

        test_strv_skip_one(STRV_MAKE("foo", "bar", "baz"), 0, STRV_MAKE("foo", "bar", "baz"));
        test_strv_skip_one(STRV_MAKE("foo", "bar", "baz"), 1, STRV_MAKE("bar", "baz"));
        test_strv_skip_one(STRV_MAKE("foo", "bar", "baz"), 2, STRV_MAKE("baz"));
        test_strv_skip_one(STRV_MAKE("foo", "bar", "baz"), 3, STRV_MAKE(NULL));
        test_strv_skip_one(STRV_MAKE("foo", "bar", "baz"), 4, STRV_MAKE(NULL));
        test_strv_skip_one(STRV_MAKE("foo", "bar", "baz"), 55, STRV_MAKE(NULL));

        test_strv_skip_one(STRV_MAKE("quux"), 0, STRV_MAKE("quux"));
        test_strv_skip_one(STRV_MAKE("quux"), 1, STRV_MAKE(NULL));
        test_strv_skip_one(STRV_MAKE("quux"), 55, STRV_MAKE(NULL));

        test_strv_skip_one(STRV_MAKE(NULL), 0, STRV_MAKE(NULL));
        test_strv_skip_one(STRV_MAKE(NULL), 1, STRV_MAKE(NULL));
        test_strv_skip_one(STRV_MAKE(NULL), 55, STRV_MAKE(NULL));
}

static void test_strv_extend_n(void) {
        _cleanup_strv_free_ char **v = NULL;

        log_info("/* %s */", __func__);

        v = strv_new("foo", "bar");
        assert_se(v);

        assert_se(strv_extend_n(&v, "waldo", 3) >= 0);
        assert_se(strv_extend_n(&v, "piep", 2) >= 0);

        assert_se(streq(v[0], "foo"));
        assert_se(streq(v[1], "bar"));
        assert_se(streq(v[2], "waldo"));
        assert_se(streq(v[3], "waldo"));
        assert_se(streq(v[4], "waldo"));
        assert_se(streq(v[5], "piep"));
        assert_se(streq(v[6], "piep"));
        assert_se(v[7] == NULL);

        v = strv_free(v);

        assert_se(strv_extend_n(&v, "foo", 1) >= 0);
        assert_se(strv_extend_n(&v, "bar", 0) >= 0);

        assert_se(streq(v[0], "foo"));
        assert_se(v[1] == NULL);
}

#if 0 /// UNNEEDED by elogind
static void test_strv_make_nulstr_one(char **l) {
        _cleanup_free_ char *b = NULL, *c = NULL;
        _cleanup_strv_free_ char **q = NULL;
        const char *s = NULL;
        size_t n, m;
        unsigned i = 0;

        log_info("/* %s */", __func__);

        assert_se(strv_make_nulstr(l, &b, &n) >= 0);
        assert_se(q = strv_parse_nulstr(b, n));
        assert_se(strv_equal(l, q));

        assert_se(strv_make_nulstr(q, &c, &m) >= 0);
        assert_se(m == n);
        assert_se(memcmp(b, c, m) == 0);

        NULSTR_FOREACH(s, b)
                assert_se(streq(s, l[i++]));
        assert_se(i == strv_length(l));
}

static void test_strv_make_nulstr(void) {
        log_info("/* %s */", __func__);

        test_strv_make_nulstr_one(NULL);
        test_strv_make_nulstr_one(STRV_MAKE(NULL));
        test_strv_make_nulstr_one(STRV_MAKE("foo"));
        test_strv_make_nulstr_one(STRV_MAKE("foo", "bar"));
        test_strv_make_nulstr_one(STRV_MAKE("foo", "bar", "quuux"));
}

static void test_strv_free_free(void) {
        char ***t;

        log_info("/* %s */", __func__);

        assert_se(t = new(char**, 3));
        assert_se(t[0] = strv_new("a", "b"));
        assert_se(t[1] = strv_new("c", "d", "e"));
        t[2] = NULL;

        t = strv_free_free(t);
}
#endif // 0

static void test_foreach_string(void) {
        const char * const t[] = {
                "foo",
                "bar",
                "waldo",
                NULL
        };
        const char *x;
        unsigned i = 0;

        log_info("/* %s */", __func__);

        FOREACH_STRING(x, "foo", "bar", "waldo")
                assert_se(streq_ptr(t[i++], x));

        assert_se(i == 3);

        FOREACH_STRING(x, "zzz")
                assert_se(streq(x, "zzz"));
}

#if 0 /// UNNEEDED by elogind
static void test_strv_fnmatch(void) {
        _cleanup_strv_free_ char **v = NULL;
        size_t pos;

        log_info("/* %s */", __func__);

        assert_se(!strv_fnmatch(STRV_MAKE_EMPTY, "a"));

        v = strv_new("xxx", "*\\*", "yyy");
        assert_se(!strv_fnmatch_full(v, "\\", 0, NULL));
        assert_se(strv_fnmatch_full(v, "\\", FNM_NOESCAPE, &pos));
        assert(pos == 1);
}
#endif // 0

int main(int argc, char *argv[]) {
#if 0 /// UNNEEDED by elogind
        test_specifier_printf();
#endif // 0
        test_str_in_set();
        test_strptr_in_set();
        test_startswith_set();
        test_strv_foreach();
        test_strv_foreach_backwards();
        test_strv_foreach_pair();
        test_strv_find();
        test_strv_find_prefix();
        test_strv_find_startswith();
        test_strv_join();
        test_strv_join_full();

        test_strv_unquote("    foo=bar     \"waldo\"    zzz    ", STRV_MAKE("foo=bar", "waldo", "zzz"));
        test_strv_unquote("", STRV_MAKE_EMPTY);
        test_strv_unquote(" ", STRV_MAKE_EMPTY);
        test_strv_unquote("   ", STRV_MAKE_EMPTY);
        test_strv_unquote("   x", STRV_MAKE("x"));
        test_strv_unquote("x   ", STRV_MAKE("x"));
        test_strv_unquote("  x   ", STRV_MAKE("x"));
        test_strv_unquote("  \"x\"   ", STRV_MAKE("x"));
        test_strv_unquote("  'x'   ", STRV_MAKE("x"));
        test_strv_unquote("  'x\"'   ", STRV_MAKE("x\""));
        test_strv_unquote("  \"x'\"   ", STRV_MAKE("x'"));
        test_strv_unquote("a  '--b=c \"d e\"'", STRV_MAKE("a", "--b=c \"d e\""));

        /* trailing backslashes */
        test_strv_unquote("  x\\\\", STRV_MAKE("x\\"));
        test_invalid_unquote("  x\\");

        test_invalid_unquote("a  --b='c \"d e\"''");
        test_invalid_unquote("a  --b='c \"d e\" '\"");
        test_invalid_unquote("a  --b='c \"d e\"garbage");
        test_invalid_unquote("'");
        test_invalid_unquote("\"");
        test_invalid_unquote("'x'y'g");

        test_strv_split();
        test_strv_split_empty();
        test_strv_split_full();
        test_strv_split_colon_pairs();
#if 0 /// UNNEEDED by elogind
        test_strv_split_newlines();
        test_strv_split_newlines_full();
#endif // 0
        test_strv_split_nulstr();
        test_strv_parse_nulstr();
#if 0 /// UNNEEDED by elogind
        test_strv_overlap();
#endif // 0
        test_strv_sort();
        test_strv_extend_strv();
#if 0 /// UNNEEDED by elogind
        test_strv_extend_strv_concat();
#endif // 0
        test_strv_extend();
#if 0 /// UNNEEDED by elogind
        test_strv_extendf();
#endif // 0
        test_strv_from_stdarg_alloca();
        test_strv_insert();
        test_strv_push_prepend();
        test_strv_push();
        test_strv_compare();
#if 0 /// UNNEEDED by elogind
        test_strv_is_uniq();
        test_strv_reverse();
        test_strv_shell_escape();
#endif // 0
        test_strv_skip();
        test_strv_extend_n();
#if 0 /// UNNEEDED by elogind
        test_strv_make_nulstr();
        test_strv_free_free();
#endif // 0

        test_foreach_string();
#if 0 /// UNNEEDED by elogind
        test_strv_fnmatch();
#endif // 0

        return 0;
}
