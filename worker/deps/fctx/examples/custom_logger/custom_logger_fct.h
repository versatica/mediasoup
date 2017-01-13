/* Extends the fct logger.

====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

File: custom_logger_fct.h
*/

#include "fct.h"

/* Define our custom logger object. */
struct _custlog_t
{
    /* Define the common logger header. */
    _fct_logger_head;
    /* Add any data members you want to maintain here. Perhaps
    you want to track something from event to event? */
};


/* Handles what to do when a fct_chk is made. */
static void
custlog__on_chk(fct_logger_i *l, fct_logger_evt_t const *e)
{
    fctchk_t const *chk = e->chk;
    fct_unused(l);
    printf("on_chk: %s\n"
           "    -  location: %s(%d)\n"
           "    -   message: %s\n"
           "    - condition: %s [DEPRECATED]\n",
           (fctchk__is_pass(chk)) ? "PASS" : "FAIL",
           fctchk__file(chk),
           fctchk__lineno(chk),
           fctchk__msg(chk),
           fctchk__cndtn(chk) /* This should be deprecated. */
          );
}



/* Handles the start of a test, for example FCT_TEST_BGN(). */
static void
custlog__on_test_start(fct_logger_i *l, fct_logger_evt_t const *e)
{
    fct_test_t const *test = e->test;
    (void)l;
    (void)e;
    printf("on_test_start:\n"
           "    -      name: %s\n",
           fct_test__name(test)
          );
}

/* Handles the end of a test, for example FCT_TEST_END(). */
static void
custlog__on_test_end(fct_logger_i *l, fct_logger_evt_t const *e)
{
    fct_test_t const *test = e->test;
    (void)l;
    printf("on_test_end:\n"
           "    -      name: %s\n"
           "    -  duration: %f ms\n",
           fct_test__name(test),
           fct_test__duration(test)
          );
}

/* Handles the start of a test suite, for example FCT_TESTSUITE_BGN(). */
static void
custlog__on_test_suite_start(fct_logger_i *l, fct_logger_evt_t const *e)
{
    fct_ts_t const *test_suite = e->ts;
    (void)l;
    printf("on_test_suite_start:\n"
           "    -      name: %s\n",
           fct_ts__name(test_suite)
          );
}

/* Handles the end of a test suite, for example FCT_TESTSUITE_END(). */
static void
custlog__on_test_suite_end(fct_logger_i *l, fct_logger_evt_t const *e)
{
    fct_ts_t const *test_suite = e->ts;
    int test_cnt = fct_ts__tst_cnt(test_suite);
    int passed_test_cnt = fct_ts__tst_cnt(test_suite);
    int failed_test_cnt = test_cnt - passed_test_cnt;
    (void)l;
    printf("on_test_suite_end:\n"
           "    -          name: %s\n"
           "    -      duration: %f ms\n"
           "    -  tests passed: %d\n"
           "    -  tests failed: %d\n"
           "    -        checks: %lu\n",
           fct_ts__name(test_suite),
           fct_ts__duration(test_suite),
           passed_test_cnt,
           failed_test_cnt,
           (unsigned long) fct_ts__chk_cnt(test_suite)
          );
}

/* Handles the first time FCTX can offically say 'start'. */
static void
custlog__on_fctx_start(fct_logger_i *l, fct_logger_evt_t const *e)
{
    (void)l;
    (void)e;
    printf("on_fctx_start:\n");
}

/* Handles the last time FCTX can do anything. */
static void
custlog__on_fctx_end(fct_logger_i *l, fct_logger_evt_t const *e)
{
    (void)l;
    (void)e;
    printf("on_fctx_end:\n");
}

/* Handles a warning message produced by FCTX. */
static void
custlog__on_warn(fct_logger_i *l, fct_logger_evt_t const *e)
{
    char const *message = e->msg;
    (void)l;
    printf("on_warn: %s\n", message);
}

/* When a conditional test suite is skipped due to conditional evaluation. */
static void
custlog__on_test_suite_skip(fct_logger_i *l, fct_logger_evt_t const *e)
{
    /* Conditional evaluation on why we skipped. */
    const char *condition = e->cndtn;
    /* Name of test suite skipped. */
    const char *name = e->name;
    (void)l;
    printf("on_test_suite_skip:\n"
           "    -      name: %s\n"
           "    - condition: %s\n",
           name,
           condition
          );
}

/* When a conditional test is skipped due to conditional evaluation. */
static void
custlog__on_test_skip(fct_logger_i *l, fct_logger_evt_t const *e)
{
    /* Conditional evaluation on why we skipped. */
    const char *condition = e->cndtn;
    /* Name of test suite skipped. */
    const char *name = e->name;
    (void)l;
    printf("on_test_suite_skip:\n"
           "    -      name: %s\n"
           "    - condition: %s\n",
           name,
           condition
          );
}

/* Handles the clean up of the logger object. Perform your special
clean up code here. */
static void
custlog__on_delete(fct_logger_i *l, fct_logger_evt_t const *e)
{
    (void)e; /* Currently doesn't supply anything. */
    puts("custlog__on_delete:\n");
    free((struct _custlog_t*)l);
}


static fct_logger_i*
custlog_new(void)
{
    struct _custlog_t *logger = (struct _custlog_t*)malloc(
                                    sizeof(struct _custlog_t)
                                );
    if ( logger == NULL )
    {
        return NULL;
    }
    fct_logger__init((fct_logger_i*)logger);
    /* The destructor. */
    logger->vtable.on_delete = custlog__on_delete;
    /* The event handlers. */
    logger->vtable.on_chk = custlog__on_chk;
    logger->vtable.on_test_start = custlog__on_test_start;
    logger->vtable.on_test_end = custlog__on_test_end;
    logger->vtable.on_test_suite_start = custlog__on_test_suite_start;
    logger->vtable.on_test_suite_end = custlog__on_test_suite_end;
    logger->vtable.on_fctx_start = custlog__on_fctx_start;
    logger->vtable.on_fctx_end = custlog__on_fctx_end;
    logger->vtable.on_warn = custlog__on_warn;
    logger->vtable.on_test_suite_skip =  custlog__on_test_suite_skip;
    logger->vtable.on_test_skip = custlog__on_test_skip;
    return (fct_logger_i*)logger;
}


/* Define our custom logger structure. */


/* Define how to install the custom logger. To override the built in
ones use the same name (i.e. "standard" to override the default logger). */
static fct_logger_types_t custlogs[] =
{
    {
        "custlog", (fct_logger_new_fn)custlog_new,
        "custom logger example, outputs everything!"
    },
    {NULL, (fct_logger_new_fn)NULL, NULL} /* Sentinel */
};


/* Redefine how to initialize FCT to automatically initialize our custom
logger. Define the _END macro for symmetry. */
#define CL_FCT_BGN()                 \
   FCT_BGN() {                       \
      fctlog_install(custlogs);

#define CL_FCT_END() } FCT_END()
