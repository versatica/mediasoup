/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

File: custom_logger.c
*/

/* Rather than include fct, we will include our 'custom_logger_fct.h'
header. This header will be responsible for exentending the logger.  */
#include "custom_logger_fct.h"

CL_FCT_BGN()
{
    FCT_QTEST_BGN(custom_logger_qtest1)
    {
        fct_chk("pass");
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(custom_logger_qtest2)
    {
        fct_chk( 1 == 1);
    }
    FCT_QTEST_END();

    FCT_SUITE_BGN(test_suite)
    {
        FCT_TEST_BGN(test1)
        {
            fct_chk( strcmp("test1", "test1")==0 );
            fct_chk( strcmp("test2", "test2")==0 );
        }
        FCT_TEST_END();

        FCT_TEST_BGN(test_will_fail)
        {
            fct_chk( 0 && "This should fail.");
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();

    printf("\n***TESTS ARE SUPPOSED TO REPORT FAILURES***\n");
    FCT_EXPECTED_FAILURES(1);

}
CL_FCT_END();
