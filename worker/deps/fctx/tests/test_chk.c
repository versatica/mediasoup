/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_basic.c

Test the Check Routines, make sure they are doing what they are supposed to do.
*/

#include "fct.h"

FCT_BGN()
{
    int chk_cnt=0;
    int req_cnt=0;
    FCT_FIXTURE_SUITE_BGN(chk_versus_req)
    {

        FCT_SETUP_BGN()
        {
        } FCT_SETUP_END();

        FCT_TEARDOWN_BGN()
        {
        } FCT_TEARDOWN_END();

        FCT_TEST_BGN(test_chk)
        {
            /* The 'chk' variant should not quite the code block,
            and instead should continue testing. */
            fct_chk( 1 ); /* Should pass */
            chk_cnt += 1;
            fct_chk( 0 ); /* Fail */
            chk_cnt += 1;
            fct_chk( 1 ); /* Pas */
            chk_cnt += 1;
            /* chk_cnt should be 3 since all lines execute. */
        }
        FCT_TEST_END();

        FCT_TEST_BGN(test_chk_not_aborted)
        {
            fct_chk( chk_cnt == 3 );
        }
        FCT_TEST_END();

        FCT_TEST_BGN(test_req)
        {
            fct_req( 1 ); /* Should pass */
            req_cnt += 1;
            fct_req( 0 ); /* Fail */
            req_cnt += 1; /* Should not executed past here... */
            fct_req( 1 ); /* Pass */
            req_cnt += 1;
        }
        FCT_TEST_END();

        FCT_TEST_BGN(test_req_aborted)
        {
            fct_chk(req_cnt == 1);
        }
        FCT_TEST_END();
    }
    FCT_FIXTURE_SUITE_END();

    printf("\n***TESTS ARE SUPPOSED TO REPORT FAILURES***\n");
    FCT_EXPECTED_FAILURES(2);
}
FCT_END();
