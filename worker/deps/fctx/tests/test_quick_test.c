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
    int count =0;   /* Use this to confirm the quick tests execute. */

    FCT_QTEST_BGN(quick)
    {
        ++count;
        fct_chk(strcmp("quick", "is quick") != 0);
    }
    FCT_QTEST_END();

    /* Quick should execute before quick_check, thus count should
    be 1. */
    FCT_QTEST_BGN(quick_check)
    {
        fct_chk(count == 1);
    }
    FCT_QTEST_END();
}
FCT_END();


