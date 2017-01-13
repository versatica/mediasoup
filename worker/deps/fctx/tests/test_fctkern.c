/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_fctkern.c

Tests for the fct kern(al) object.
*/

#include "fct.h"

FCT_BGN()
{

    FCT_QTEST_BGN(fctkern_test_add_prefix_filter)
    {
        /* See bug #499089 */
        char const *argv_dummy[] = {"test"};
        int argc_dummy = 1;
        fctkern_t k;
        fctkern__init(&k, argc_dummy, argv_dummy);
        fctkern__add_prefix_filter(&k, "aaaa");
        fct_chk( fctkern__filter_cnt(&k) == 1 );
        fct_chk( fctkern__pass_filter(&k, "aaaa") );
        fct_chk( !fctkern__pass_filter(&k, "aaab") );
    }
    FCT_QTEST_END();

}
FCT_END();

