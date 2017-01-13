/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_basic.c

Tries to catch a segmentation fault that occurs with a simple test case,
and invoking the --help option. First reported by RhysU.
*/

#include "fct.h"

FCT_BGN()
{

    FCT_QTEST_BGN(debug)
    {
        fct_chk(1);
    }
    FCT_QTEST_END();

}
FCT_END();
