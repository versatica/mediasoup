/*
====================================================================
Copyright (c) 2009 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

File: test_call_teardown.c

Checks that the teardown function is executed (bug #382628).
*/

#include "fct.h"

FCT_BGN()
{
    /* This value is incremented during the setup, then incremented in the
    first test, it is finally decremented in the last test. Thus in the next
    test suite I can test for a value of 1. */
    int reference =0;
    FCT_FIXTURE_SUITE_BGN("1Suite")
    {
        FCT_SETUP_BGN()
        {
            reference++;
        }
        FCT_SETUP_END();

        FCT_TEARDOWN_BGN()
        {
            reference--;
        }
        FCT_TEARDOWN_END();

        FCT_TEST_BGN(__dummy__)
        {
            reference++;
        }
        FCT_TEST_END();
    }
    FCT_FIXTURE_SUITE_END();

    FCT_SUITE_BGN(check_teardown)
    {
        FCT_TEST_BGN(check_reference_count)
        {
            fct_chk( reference == 1 );
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();

}
FCT_END();
