/*
====================================================================
Copyright (c) 2010 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_fct_chk_conditional.c

This code is designed to fail. It runs tests to confirm that failure conditions are failing properly.
*/

#include "fct.h"

FCT_BGN()
{

    FCT_QTEST_BGN(fct_chk_if)
    {
        if ( fct_chk(1) )
        {
            fct_chk(1);
        }
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(fct_xchk_if)
    {
        if ( fct_xchk(1, "") )
        {
            fct_chk(1);
        }
    }
    FCT_QTEST_END();

}
FCT_END();
