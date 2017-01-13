/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_basic.c

Provides a dumping ground for basic tests of fct.
*/

#include "fct.h"

FCT_BGN()
{
    /* These test suites share the same name. */
    FCT_SUITE_BGN(simple)
    {
        /* A test, simply check that 1 is still 1. */
        FCT_TEST_BGN(simple__one_is_one)
        {
            fct_chk(1);
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();
    FCT_SUITE_BGN(simple)
    {
        /* A test, simply check that 1 is still 1. */
        FCT_TEST_BGN(simple_two_is_two)
        {
            fct_chk(2);
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();

    /* This test has two tests with the same name. */
    FCT_SUITE_BGN(same_name_tests)
    {
        FCT_TEST_BGN(same_name)
        {
            fct_chk(1);
        }
        FCT_TEST_END();
        FCT_TEST_BGN(same_name)
        {
            fct_chk(1);
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();

}
FCT_END();


