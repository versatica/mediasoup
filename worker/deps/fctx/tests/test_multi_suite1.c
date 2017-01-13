/*
====================================================================
Copyright (c) 2009 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_multi_suite1.c
*/

#include "fct.h"

#include <string.h>

/* Original Multi-file case would fall down on this since it would
end up including a function definition within main() {}. */
static int
streq(char const *c1, char const *c2)
{
    return strcmp(c1, c2) ==0;
}

FCTMF_SUITE_BGN(test_suite1)
{
    FCT_TEST_BGN(simple__one_is_one)
    {
        fct_chk(1);
    }
    FCT_TEST_END();
    FCT_TEST_BGN(simple__check_streq)
    {
        fct_chk(streq("one", "one"));
        fct_chk(!streq("one", "two"));
    }
    FCT_TEST_END();
}
FCTMF_SUITE_END()
