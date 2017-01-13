=====================
FCTMF: Multi-file API
=====================

.. module:: FCTMF
   :platform: Unix, Windows
   :synopsis: FCTX across multiple files.

.. versionadded:: 1.1
   The FCTX Multi-File (FCTMF) supersedes the older "include method"
   described in version 1.0.

Introduction
------------

When you start unit testing with FCTX you will find that at some point
holding all your tests in one file can become unwieldy. The Multi-File api
(FCTMF) attempts to provide a minimal barrier to writing unit tests that span
multiple files.

The idea is to have one file that starts up tests defined in each of the other
files. The process of registration is a one step "call" into the other files.

Multi-File Setup
----------------

Imagine we have two files: :file:`test_main.c` and :file:`test_a.c`. In
:file:`test_main.c` we will setup and register our test that is defined in
:file:`test_a.c`.

Lets look at :file:`test_main.c`,

.. code-block:: c
   
   /* file: test_main.c */

   #include "fct.h"
   
   FCT_BGN() {
       FCTMF_SUITE_CALL(test_a_feature);
   } FCT_END();

.. /* fixes vim highlighting. 

there now we have set it up to call into the test suite called "test_a_feature"
defined in :file:`test_a.c`.  Lets look at the contents of :file:`test_a.c`.

.. code-block:: c

   /* file: test_a.c */

   #include "fct.h"

   FCTMF_SUITE_BGN(test_a_feature) {

       FCT_TEST_BGN(sanity_check) {
           fct_chk( a_feature__sanity_check() );
       } FCT_TEST_END();

   } FCTMF_SUITE_END();

.. /* fixes vim highlighting. 


this will register a test suite called "test_a_feature" and execute a test
called "sanity_check". 

If you wanted to use a test suite with fixtures (setup/teardown), you would do
the following,

.. code-block:: c

   /* file: test_a.c */
 
   #include "fct.h"

   static a_object_t *obj; 

   FCTMF_FIXTURE_SUITE_BGN(test_a_feature) {

       FCT_SETUP_BGN() {
           obj = a_object_new();
       } FCT_SETUP_END();

       FCT_TEARDOWN_BGN() {
           a_object__del(obj);
           obj =NULL;
       } FCT_TEARDOWN_END();

       FCT_TEST_BGN(sanity_check) {
           fct_chk( a_object__sanity_check(obj) );
       } FCT_TEST_END()

   } FCTMF_FIXTURE_SUITE_END();

.. /* (Just fixes VIM highlighter)

the only difference here being the introduction of "FIXTURE" into scope
statements as well as the SETUP and TEARDOWN fixtures themselves.

The key thing to also notice is that all the testing, checking, setup and
teardown macros follow the existing :mod:`FCT` module.

Note for MVC Compilers
----------------------

Using the FCTMF API with warning level 4 will produce the following::

    warning C4210: nonstandard extension used : function given file scope

so far testing both with MVC and GCC FCTMF has yet to fail, except for this
warning level above.

So what's happening here?

The FCTMF_SUITE_CALL uses the following little trick whereby,

.. code-block:: c

   FCTMF_SUITE_CALL(my_test_suite);

becomes,

.. code-block:: c

   void my_test_suite(fctkern_t *fk);
   my_test_suite(fctkern_ptr__);


.. /* (Just fixes VM highlighter)

where we make a "variable" and "run it", and let the linker sort it out all in
the end.

The goal here was to prevent you from having to repeatedly "register" your test
suite in order for you get up and running. To stay at warning level 4, but
quite the compiler, you can do the following,

.. code-block:: c

    #if defined(_MSC_VER) 
    #   pragma warning(push, 3)
    #endif /* _MSC_VER */
        FCTMF_SUITE_CALL(my_test_suite);
        FCTMF_SUITE_CALL(test_fixture_suite2);
    #if defined(_MSC_VER)
    #   pragma warning(pop)
    #endif /* _MSC_VER */

.. /* (Just fixes VM highlighter)

and you will be able to continue to use /W4 with MVC as well as GCC.

   
Multi-File Test Suites
----------------------

.. c:function:: FCTMF_SUITE_CALL(name)

        This launches the test suite defined by *name*. You would place this
        call between the :c:func:`FCT_BGN()` and :c:func:`FCT_END()` scope. This
        simply calls off to another file, and it does not prevent you from
        having other tests within the :c:func:`FCT_BGN()` and :c:func:`FCT_END()`
        scope.

        For Visual Studio 6 compilers, you will need to use the
        :c:func:`FCTMF_SUITE_DEF` function.

.. c:function:: FCTMF_FIXTURE_SUITE_BGN(name)
	
	Following the xtest convention, every test suite needs to start with a 
	SUITE_BGN function. In by using the FIXTURE variants you are indicating
	that you wish to install a SETUP and TEARDOWN fixture via the
	:c:func:`FCT_SETUP_BGN` and :c:func:`FCT_SETUP_END` and
	:c:func:`FCT_TEARDOWN_BGN` and :c:func:`FCT_TEARDOWN_END` functions.

	See also :c:func:`FCTMF_SUITE_BGN`.

.. /*  (Just fixes VIM highlighter)

.. c:function:: FCTMF_FIXTURE_SUITE_END(name)

	This closes a test suite that contains fixtures. If you do not wish to
	specify a setup/teardown you would use the :c:func:`FCT_SUITE_END` 
	function instead.

.. c:function:: FCTMF_SUITE_BGN(name)

        Use this FCTMF_SUITE variant if you do not want to bother specifying a
        SETUP and TEARDOWN blocks.

        See also :c:func:`FCTMF_FIXTURE_SUITE_BGN`.

.. c:function:: FCTMF_SUITE_END()

        Closes the :c:func:`FCTMF_SUITE_BGN` function.

.. c:function:: FCTMF_SUITE_DEF(name)

        Defines a test suite with the given *name*. This is only required for
        the Visual Studio 6 compilers (or if you choose to avoid the comments
        in `Note for MVC Compilers`_). This call must be made *before* the
        :c:func:`FCT_BGN` function. The *name* must be the same as the name in
        :c:func:`FCTMF_SUITE_CALL` and :c:func:`FCTMF_SUITE_BGN`.
