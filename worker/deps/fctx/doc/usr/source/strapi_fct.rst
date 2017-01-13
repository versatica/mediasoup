==================
fctstr: String API
==================

.. module:: fctstr
   :platform: Unix, Windows
   :synopsis: Basic string API to support unit testing.

.. versionadded:: 1.2
   API for string operations that you may encounter during unit testing.

Introduction
------------

Provides an API for common string operations that you may encounter during unit
testing. This module will grow over time.

String Functions
----------------

.. c:function:: fctstr_clone(s)

        Allocates new memory, and copies the contents of *s* into the result.
        Callee should use the standard C Runtime Library's *free* function to
        clean up the memory.

.. c:function:: fctstr_clone_lower(s)

        *New in FCTX 1.3.* Same as :c:func:`fctstr_clone`, but returns the
        string, *s*, as a lower case string. 

.. c:function:: fctstr_eq(s1,s2)

        Returns a non-zero (TRUE) if *s1* and *s2* are equal. Unlike the
        standard strcmp function, this function can handle NULL values. This
        function will also quit the moment one of the strings is unequal. If
        *s1* and *s2* are NULL, this function will return non-zero (TRUE)
        value. See :c:func:`fctstr_ieq` for a case-insensitive string compare.

.. c:function:: fctstr_endswith(s, check)

        *New in FCTX 1.3*. Returns true if the string, *s*, ends with
        the given *check* string.

.. c:function:: fctstr_ieq(s1,s2) 
         
        Performs a case insensitve string compare, see :c:func:`fctstr_eq` for
        more details.

.. c:function:: fctstr_iendswith(s, check)

        *New in FCTX 1.3.* Case insensitive variant of
        :c:func:`fctstr_endswith`.

.. c:function:: fctstr_incl(s1, check)

        *New in FCTX 1.3.* Returns true if the sub-string, *check*, can
        be found in the string *s1*.  This function will change NULL input
        values for a blank string before proceeding to call the standard
        C *strstr* function.

.. c:function:: fctstr_iincl(s1, check)

        *New in FCTX 1.3.* Case insensitive version of
        :c:func:`fctstr_incl`.

.. c:function:: fctstr_istartswith(s1, check)

        *New in FCTX 1.3*. Case insensitive variant of
        :c:func:`fctstr_startswith`. 


.. c:function:: fctstr_startswith(s1, check)

        *New in FCTX 1.3*. Returns true (1) if the string, *s1*, starts
        with the given string, *check*. This will treat two NULL values
        as equal, and return true (1). 

