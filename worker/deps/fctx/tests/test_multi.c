/*
====================================================================
Copyright (c) 2009 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_multi.c

This code is designed to experiment with the "multi-file" support
for the FCT.

This method superscedes the test_mutli_old_style method shown in the other
test example. As it doesn't suffer from some of the foibles in that
method.
*/

#include "fct.h"

/* These no longer do anything, except for the older Visual Studio 6
compiler. */
FCTMF_SUITE_DEF(test_suite1);
FCTMF_SUITE_DEF(test_fixture_suite2);

FCT_BGN()
{
    /* This suite is called *OUTSIDE* of this compilation unit. The
    pragma's are to tackle a /W4 problem on MVC compilers. */
#if defined(_MSC_VER)
#   pragma warning(push, 3)
#endif /* _MSC_VER */
    FCTMF_SUITE_CALL(test_suite1);
    FCTMF_SUITE_CALL(test_fixture_suite2);
#if defined(_MSC_VER)
#   pragma warning(pop)
#endif /* _MSC_VER */

    /* Provide a co-existing "embedded" version for completness. */
    FCT_SUITE_BGN(test_embedded)
    {
        FCT_TEST_BGN(test_embedded.zero_is_zero)
        {
            fct_chk( 0 == 0 );
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();
}
FCT_END();
