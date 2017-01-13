/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_multi_old_style.c

This method is superceded by the test_multi method as this method
suffers from some serious annoying behaviour. Namely your included
files can not themselves INCLUDE any files NOR define any helper
functions.
*/

#include "fct.h"

FCT_BGN()
{
#include "test_multi_old_style_suite_a.h"
#include "test_multi_old_style_suite_b.h"
}
FCT_END();

