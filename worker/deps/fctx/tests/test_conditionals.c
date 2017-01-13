/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

File: test_1suite.c

Tests that the conditionals are working correctly.  Runs through a bunch of
tests to see if certain flags where set, if they where done corrrectly then the
subsequent "real" unit test will work just fine.

*/

#include "fct.h"

FCT_BGN()
{
    int true_condition =1;
    int false_condition =0;
    int is_test_suite_fixture_if_true =0;
    int is_test_suite_fixture_if_false =0;
    int is_test_suite_if_true =0;
    int is_test_suite_if_false =0;
    int is_qtest_if_true =0;
    int is_qtest_if_false =0;
    int is_test_if_true =0;
    int is_test_if_false =0;

    /* ------------------------------------------------------------- */

    FCT_FIXTURE_SUITE_BGN_IF(true_condition, test_suite_if_true)
    {
        FCT_SETUP_BGN()
        {
        } FCT_SETUP_END();
        FCT_TEARDOWN_BGN()
        {
        } FCT_TEARDOWN_END();
        FCT_TEST_BGN(run_fixture_test_if_true)
        {
            is_test_suite_fixture_if_true =1;
        }
        FCT_TEST_END();
    }
    FCT_FIXTURE_SUITE_END_IF();

    FCT_FIXTURE_SUITE_BGN_IF(false_condition, test_suite_if_false)
    {
        FCT_SETUP_BGN()
        {
        } FCT_SETUP_END();
        FCT_TEARDOWN_BGN()
        {
        } FCT_TEARDOWN_END();
        FCT_TEST_BGN(run_fixture_test_if_false)
        {
            is_test_suite_fixture_if_false =1;
        }
        FCT_TEST_END();
    }
    FCT_FIXTURE_SUITE_END_IF();

    /* ------------------------------------------------------------- */

    FCT_SUITE_BGN_IF(true_condition, test_suite_if_true)
    {
        FCT_TEST_BGN(run_test_if_true)
        {
            is_test_suite_if_true =1;
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END_IF();

    FCT_SUITE_BGN_IF(false_condition, test_suite_if_false)
    {
        FCT_TEST_BGN(run_test_if_false)
        {
            is_test_suite_if_false =1;
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END_IF();

    /* ------------------------------------------------------------- */

    FCT_QTEST_BGN_IF(true_condition, test_qtest_if_true)
    {
        is_qtest_if_true = 1;
    }
    FCT_QTEST_END_IF();

    FCT_QTEST_BGN_IF(false_condition, test_qtest_if_false)
    {
        is_qtest_if_false = 1;
    }
    FCT_QTEST_END_IF();

    /* ------------------------------------------------------------- */

    FCT_SUITE_BGN(test_if_true)
    {
        FCT_TEST_BGN_IF(true_condition, run_test_if_true)
        {
            is_test_if_true =1;
        }
        FCT_TEST_END_IF();
    }
    FCT_SUITE_END_IF();

    FCT_SUITE_BGN(test_if_false)
    {
        FCT_TEST_BGN_IF(false_condition, run_test_if_false)
        {
            is_test_if_false =1;
        }
        FCT_TEST_END_IF();
    }
    FCT_SUITE_END();

    /* ------------------------------------------------------------- */
    puts("--- Confirm Conditionals Worked -- \n");

    FCT_QTEST_BGN(confirm_conditionals__fixture_suite)
    {
        fct_chk( is_test_suite_fixture_if_true );
        fct_chk( !is_test_suite_fixture_if_false );
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(confirm_conditionals__nofixture_suite)
    {
        fct_chk( is_test_suite_if_true );
        fct_chk( !is_test_suite_if_false );
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(confirm_conditionals__qtest)
    {
        fct_chk( is_qtest_if_true );
        fct_chk( !is_qtest_if_false );
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(confirm_conditionals__test)
    {
        fct_chk( is_test_if_true );
        fct_chk( !is_test_if_false );
    }
    FCT_QTEST_END();
}
FCT_END();

