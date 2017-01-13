/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_fail.c

This code is designed to fail. It runs tests to confirm that failure conditions are failing properly.
*/

#include "fct.h"

FCT_BGN()
{
    /* A very simple test suite, it doesn't have any data to
    setup/teardown. */
    FCT_SUITE_BGN(should_fail)
    {
        /* A test, simply check that 1 is still 1. */
        FCT_TEST_BGN(false is fail)
        {
            fct_chk(0);
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();

}
FCT_END();


