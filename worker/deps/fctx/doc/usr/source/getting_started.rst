===============
Getting Started
===============


To get you started quickly you only need to have the "fct.h" header file
somewhere handy on your system. In the distribution package, it can be found in
the "include" directory. 

What follows an overly simple example of testing. 

.. code-block:: c

  /* First include the fct framework. */
  #include "fct.h"

  /* Include your API. In this case we are going to test strcmp. */
  #include <string.h>

  /* Now lets define our testing scope. */
  FCT_BGN()
  {
    /* An actual test case in the test suite. */
    FCT_QTEST_BGN(strcmp_eq)
    {
       fct_chk(strcmp("durka", "durka") == 0);
    }
    FCT_QTEST_END();


    FCT_QTEST_BGN(chk_neq)
    {
       fct_chk(strcmp("daka", "durka") !=0 );
    }
    FCT_QTEST_END();

  /* Every FCT scope has an end. */
  }
  FCT_END();

Now you can compile the above file and generate a new EXE. If your new
EXE was called "test.exe", you can run it with a "filtering prefix" to
limit the tests executed, for example::

   test strcmp_eq

would only execute the "strcmp_eq" test.

To define a SETUP/TEARDOWN structure you would do something similar to
the above tests but this time we would introduce a test suite.

.. code-block:: c

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


