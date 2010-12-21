// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#include <stdlib.h>
#include <assert.h>
#include <junkie/cpp.h>
#include "tools/ip_addr.c"

static struct ip_addr const a = { .family = AF_INET, .u = { .v4 = { 0x0101A8C0 } }};
static struct ip_addr const b = { .family = AF_INET, .u = { .v4 = { 0x0201A8C0 } }};

static void ip_addr_cmp_check_(struct ip_addr const *a, struct ip_addr const *b, int expected)
{
    int cmp = ip_addr_cmp(a, b);
    assert(cmp == expected);
}

static void ip_addr_check(void)
{
    ip_addr_cmp_check_(&a, &a, 0);
    ip_addr_cmp_check_(&a, &b, -1);
    ip_addr_cmp_check_(&b, &a, 1);

    assert(!ip_addr_is_v6(&a) && !ip_addr_is_v6(&b));
}

static void ip_addr_ctor_from_str_check(void)
{
    static struct {
        char const *str;
        int mode;
    } const tests[] = {
        { "0.0.0.0",        4, },
        { "1.2.3.4",        4, },
        { "0.0.0.1",        4, },
        { "128.2.1.255",    4, },
        { "::ffff:1.2.3.4", 6, },
    };

    for (unsigned t = 0; t < NB_ELEMS(tests); t++) {
        struct ip_addr addr;
        ip_addr_ctor_from_str(&addr, tests[t].str, strlen(tests[t].str), tests[t].mode );
        char const *str = ip_addr_2_str(&addr);
        SLOG(LOG_DEBUG, "Comparing '%s' with '%s'", tests[t].str, str);
        assert(0 == strcmp(str, tests[t].str));
    }
}

static void ip_addr_routable_check(void)
{
    static struct {
        char const *str;
        int mode;
        bool routable;
    } const tests[] = {
        { "0.0.0.0",        4, true },
        { "1.2.3.4",        4, true },
        { "0.0.0.1",        4, true },
        { "128.2.1.255",    4, true },
        { "::ffff:1.2.3.4", 6, true },
        { "127.0.0.1",      4, false },
        { "172.24.5.4",     4, false },
        { "192.168.10.9",   4, false },
    };

    for (unsigned t = 0; t < NB_ELEMS(tests); t++) {
        struct ip_addr addr;
        ip_addr_ctor_from_str(&addr, tests[t].str, strlen(tests[t].str), tests[t].mode);
        assert(ip_addr_is_routable(&addr) == tests[t].routable);
    }
}

static void broadcast_check(void)
{
    struct {
        char const *str;
        uint32_t netmask;
        bool is_broadcast;
    } tests[] = {
        { "1.0.0.0",        0xff000000U, false },
        { "127.0.0.1",      0xff000000U, false },
        { "128.10.5.255",   0xffff0000U, false },
        { "192.168.10.9",   0xffffff00U, false },
        { "10.255.255.255", 0xff000000U, true  },
        { "127.0.255.255",  0xff000000U, false },
        { "128.0.255.255",  0xffff0000U, true  },
        { "192.168.10.255", 0xffffff00U, true  },
    };

    for (unsigned t = 0; t < NB_ELEMS(tests); t++) {
        struct ip_addr addr;
        ip_addr_ctor_from_str(&addr, tests[t].str, strlen(tests[t].str), 4);
        assert(netmask_of_address(addr.u.v4) == tests[t].netmask);
        assert(ip_addr_is_broadcast(&addr) == tests[t].is_broadcast);
    }
}

int main(void)
{
    log_init();
    log_set_level(LOG_DEBUG, NULL);
    log_set_file("ip_addr_check.log");

    ip_addr_check();
    ip_addr_ctor_from_str_check();
    ip_addr_routable_check();
    broadcast_check();

    log_fini();
    return EXIT_SUCCESS;
}
