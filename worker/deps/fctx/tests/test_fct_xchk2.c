/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

File: test_fct_xchk2
*/

#include "fct.h"

FCT_BGN()
{
    FCT_QTEST_BGN(three_names)
    {
        const char *test_names[] =
        {
            "test_1",
            "test_2",
            "test_3",
            NULL
        };
        const char **itr;
        for ( itr = test_names; *itr != NULL; ++itr )
        {
            fct_xchk2(
                *itr,
                1,
                "always succeed"
            );
        }
    }
    FCT_QTEST_END();
}
FCT_END();


