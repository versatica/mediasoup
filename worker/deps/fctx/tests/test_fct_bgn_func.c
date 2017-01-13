/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

File: test_fct_bgn_func.c
*/

#include "fct.h"

FCT_BGN_FN(function_entry)
{
    FCT_QTEST_BGN(test_func)
    {
        fct_chk(1);
    }
    FCT_QTEST_END();
}
FCT_END_FN();


int main(int argCount, char *argVariables[])
{
    /* We could supply our own argc, argv, but for testing purposes
    supplying the standard ones are OK. */
    return function_entry(argCount, argVariables);
}

