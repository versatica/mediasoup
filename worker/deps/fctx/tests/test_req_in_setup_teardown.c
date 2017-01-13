/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_req_in_setup_teardown.c

Tests running fct_req within a setup and teardown.
*/

#include "fct.h"

FCT_BGN()
{
    int is_abort_setup =1; /* Assume success unless otherwise noted. */
    int is_abort_teardown =1;

    FCT_FIXTURE_SUITE_BGN(check_in_setup)
    {
        FCT_SETUP_BGN()
        {
            fct_req(0 && "cause a failure during setup");
            is_abort_setup =0;
        }
        FCT_SETUP_END();

        FCT_TEARDOWN_BGN()
        {
        } FCT_TEARDOWN_END();
    }
    FCT_FIXTURE_SUITE_END();

    FCT_FIXTURE_SUITE_BGN(check_in_teardown)
    {
        FCT_SETUP_BGN()
        {
        } FCT_SETUP_END();

        FCT_TEARDOWN_BGN()
        {
            fct_req(0 && "cause a failure during teardown");
            is_abort_teardown =0;
        }
        FCT_TEARDOWN_END();
    }
    FCT_FIXTURE_SUITE_END();

    FCT_QTEST_BGN(verify_we_aborted_in_setup)
    {
        fct_chk(is_abort_setup == 1);
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(verify_we_aborted_in_teardown)
    {
        fct_chk(is_abort_teardown == 1);
    }
    FCT_QTEST_END();

    printf("\n***TESTS ARE SUPPOSED TO REPORT FAILURES***\n");
    FCT_EXPECTED_FAILURES(2);
}
FCT_END();

