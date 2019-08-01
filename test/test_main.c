//
// Created by mjonsson on 7/16/19.
//

#include "syno_lib.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <malloc.h>

static void test_init(void** state)
{
    int err = init("/home/mjonsson/dev/syno_com/server.conf", "|!\n");
    assert_int_equal(err, 0);
    assert_int_not_equal(6, 4);
}

static void test_login(void** state)
{
    init("/home/mjonsson/dev/syno_com/server.conf", "|!\n");
    int err = test_connection("FileStation");
    assert_int_not_equal(err, 0);
}

//static void test_query_all(void** state)
//{
//    init("/home/mjonsson/dev/syno_com/server.conf", "|!\n");
//    void* rsp = malloc(RSP_NUM_BYTES);
//    int err = make("FileStation", "/webapi/query.cgi?api=SYNO.API.Info&version=1&method=query&query=all", &rsp);
//    free(rsp);
//    assert_int_equal(err, 0);
//}

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
        cmocka_unit_test(test_init),
        cmocka_unit_test(test_login),
        //cmocka_unit_test(test_query_all)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

