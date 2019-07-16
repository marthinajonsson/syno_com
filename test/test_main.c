//
// Created by mjonsson on 7/16/19.
//

#include "syno_lib.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

static void test(void** state)
{
    init("/home/mjonsson/projs/syno_com/server.conf");
    assert_int_equal(3, 3);
    assert_int_not_equal(4, 4);
}

int setup (void ** state)
{
    return 0;
}

int teardown (void ** state)
{
    return 0;
}

int main(void)
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}