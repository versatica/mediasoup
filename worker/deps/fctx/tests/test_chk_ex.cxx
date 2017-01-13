/*
====================================================================
Copyright (c) 2011 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_chk_ex.cxx
*/

#include "fct.h"

class err_t {};
class sub_err_t : public err_t {};
class other_err_t {};

static void throw_err() {
   throw err_t();
}

static void throw_sub_err() {
   throw sub_err_t();
}

static void throw_other_err() {
   throw other_err_t();
}

static void not_going_to_throw() {
  int j = 2+1;
  char buf[32];
  fct_snprintf(buf, sizeof(buf), "j=%d", j);
}

FCT_BGN()
{
    FCT_QTEST_BGN(throw_err) {
       fct_chk_ex(
           err_t&, 
           throw_err()
       );
    } FCT_QTEST_END();


    FCT_QTEST_BGN(throw_and_catch_sub_err) {
       fct_chk_ex(
           sub_err_t&, 
           throw_sub_err()
       );
    } FCT_QTEST_END();

    FCT_QTEST_BGN(throw_and_catch_other_err__should_fail) {
       /* This is checking for an exception of type "sub_err_t", but
       doesn't get it. Should fail! */
       fct_chk_ex(
           sub_err_t&,
           throw_other_err()
       );
    } FCT_QTEST_END();

    FCT_QTEST_BGN(doesnt_throw) {
       /* This is expecting the function to throw an error, but it
       doesn't. */
       fct_chk_ex(
           err_t&,
           not_going_to_throw();
       );
    } FCT_QTEST_END();

    printf("\n***TESTS ARE SUPPOSED TO REPORT FAILURES***\n");
    FCT_EXPECTED_FAILURES(2);    
} 
FCT_END();
