/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

Tests "counting" of various elements of a unit test framework. For
example the number of tests, the number of checks, &etc.

TODO - Run this in verbose mode. This is currently waiting for
the verbose mode to be released.
*/

/* To activate some "gut check" functions that are not used normally. */
#define FCT_USE_TEST_COUNT
#include "fct.h"

FCT_BGN()
{
    /* An empty test suite that doesn't have any tests within
    it, then confirm that at our count is still zero. */
    FCT_SUITE_BGN(empty)
    {
    }
    FCT_SUITE_END();
    _FCT_GUTCHK(fctkern__tst_cnt(fctkern_ptr__) == 0);

    /* Now run a suite again, but place one test in it. Then confirm with
    our gut check. */
    FCT_SUITE_BGN(one_test)
    {
        FCT_TEST_BGN(check_one_test)
        {
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();
    _FCT_GUTCHK(fctkern__tst_cnt(fctkern_ptr__) == 1);
    _FCT_GUTCHK(fctkern__chk_cnt(fctkern_ptr__) == 0);

    /* Now again, but with two checks. The gut check should return 3 for the
    number of tests. */
    FCT_SUITE_BGN(one_test_again)
    {
        FCT_TEST_BGN(check_one_test_again)
        {
            fct_chk(1);
            fct_chk(1);
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();
    _FCT_GUTCHK(fctkern__tst_cnt(fctkern_ptr__) == 2);
    _FCT_GUTCHK(fctkern__chk_cnt(fctkern_ptr__) == 2);


}
FCT_END();


