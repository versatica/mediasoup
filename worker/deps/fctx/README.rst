===============================
FCTX (FAST C XUNIT TEST) README
===============================

--------
Overview
--------

Fast C Test (FCTX) is an all in one header file that lets you create
tests fast in C/C++. With FCTX you can write and run a test as fast as
you can think about it. There is no need to write a test and register a
test. Once the test is written it will be executed at runtime. There is
no need for post-processing to generate a compilable file, you can run
and view the results. The FCTX only requires a ANSI C/C++ compiler to
run, there are no third party requirements.

Simply include, write your test, and compile.

The FCTX principle design goal is to make it possible to quickly
generate unit tests for C and C++ with little or no hassle.

It is just a header, you only need #include.

----------
What's New
----------

See the NEWS file for release notes.

---------
Licensing
---------

The official statement:

   Copyright (c) 2008 Ian Blumel.  All rights reserved.

   This software is licensed as described in the file LICENSE, which
   you should have received as part of this distribution.

What that LICENSE file is essentially stating in plain English:

   Please be a good, smart person, and use due diligence. I can make no
   claims that what you are using will not light your computer on fire.
   I can make no claims that everything written in FCTX will be 100%
   correct. 
   
   All I ask is that if FCT is extended or redistributed, that the
   original contributors be notified, and that a proper reference is
   appreciated.

Thanks.

-----------------
Quick Start Guide
-----------------

To get you started quickly you only need to have the "fct.h" header file
somewhere handy on your system. In the distribution package, it can be
found in the "include" directory. 

What follows an overly simple example of testing::

  /* First include the fct framework. */
  #include "fct.h"

  /* Include your API. In this case we are going to test strcmp. */
  #include <string.h>

  /* Now lets define our testing scope. */
  FCT_BGN()
  {
     /* A simple test case. No setup/teardown. */
     FCT_SUITE_BGN(simple)
     {
        /* An actual test case in the test suite. */
        FCT_TEST_BGN(strcmp_eq)
        {
           fct_chk(strcmp("durka", "durka") == 0);
        }
        FCT_TEST_END();


        FCT_TEST_BGN(chk_neq)
        {
           fct_chk(strcmp("daka", "durka") !=0 );
        }
        FCT_TEST_END();


     /* Every test suite must be closed. */
     }
     FCT_SUITE_END();   

  /* Every FCT scope has an end. */
  }
  FCT_END();

Now you can compile the above file and generate a new EXE. If your new
EXE was called "test.exe", you can run it with a "filtering prefix" to
limit the tests executed, for example::

   test strcmp_eq

would only execute the "strcmp_eq" test.

To define a SETUP/TEARDOWN structure you would do something similar to
the above tests::

  FCT_BGN()
  {
     /* Optionally, define a scope for you data. */
     {
        void *data =NULL;
        FCT_FIXTURE_SUITE_BGN(Fixtures)
        {
              FCT_SETUP_BGN()
              {
                 /* Initialize your data before a test is executed. */
                 data = malloc(sizeof(10));
              }
              FCT_SETUP_END();

              FCT_TEARDOWN_BGN()
              {
                 /* Clean up your data after a test is executed. */
                 free(data);
                 data = NULL;
              }
              FCT_TEARDOWN_END();

              FCT_TEST_BGN(silly_test_for_null)
              {
                 fct_chk( data != NULL );
              }
              FCT_TEST_END();

              FCT_TEST_BGN(silly_test_for_null__again)
              {
                 fct_chk( data != NULL );
              }
              FCT_TEST_END();
              
        }
        FCT_FIXTURE_SUITE_END();
     }
  }
  FCT_END();

Afterwards, you can compile and run this test, and the "data" will be
setup and teared down after each test cycle.

----------------
Development Goal
----------------

To state it out loud: FCT is dedicated to reducing the overhead
associated with generating tests in C and C++.

--------
Building
--------

Build your Own Test Suite
-------------------------

To build your own test suite: Its just a header, include into your test 
file, and run the compiler. 

Build the FCT Tests
-------------------

To build the tests themselves: use CMAKE (http://www.cmake.org/). On Linux
or similar system do something like this (from the root source directory)::

   mkdir build
   cd build
   cmake ../

At this point you should have a Makefile in your "build" directory. 
Type "make help" for a list of targets.

On a Win32 Machine it depends what you want to ultimately work with. 
The following example illustrates creating a Visual Studio 9 solution::

   mkdir msw
   cd msw
   cmake -G"Visual Studio 9 2008" ..\

At this point you should have a FCT.sln file within your MSW directory.
If you wanted to generate a different project, type::

   cmake --help

To get list of generators. For example, if you wanted to use MinGW, 
you could do something like::

   mkdir mingw
   cmake -G"MinGW Makefiles" ..\

and now you will have a Makefile configured to compile with MinGW.
