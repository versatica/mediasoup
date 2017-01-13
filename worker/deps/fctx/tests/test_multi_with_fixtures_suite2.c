/*
====================================================================
Copyright (c) 2009 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_multi_with_fixtures_suite2.c

This file has a test suite with a fixture.
*/

#include "fct.h"
#include <string.h>

/* Original Multi-file case would fall down on this since it would
end up including a function definition within main() {}. */
static int
streq(char const *c1, char const *c2)
{
    return strcmp(c1, c2) ==0;
}

FCTMF_FIXTURE_SUITE_BGN(test_fixture_suite2)
{
    /* Fixture Variables. */
    int count =0;

    FCT_SETUP_BGN()
    {
        ++count;
    }
    FCT_SETUP_END();
    FCT_TEARDOWN_BGN()
    {
        --count;
    }
    FCT_TEARDOWN_END();

    FCT_TEST_BGN(suite2.check_one_is_one)
    {
        fct_chk(1);
    }
    FCT_TEST_END();

    FCT_TEST_BGN(suite2.check_eq_str)
    {
        fct_chk( streq("hello", "hello") );
    }
    FCT_TEST_END();

    FCT_TEST_BGN(suite2.check_setup_teardown)
    {
        /* The count should always be one if the
        setup and teardown are doing their job. */
        fct_chk( count == 1 );
    }
    FCT_TEST_END();

}
FCTMF_FIXTURE_SUITE_END();
