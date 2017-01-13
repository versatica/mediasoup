/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

File: test_clp.c

Runs through tests for the command line parser.
*/


#include "fct.h"


/* Test Data */
fct_clp_t clp;
fctcl_init_t options[] =
{
    /* The "casting to char*" is bad mojo here. But until I
        grow a "fctcl_init_t" object with constants, it will
        do to quiet down C++. It turns out that you never delete
        this data, so it is OK to cast it to a char*. */
    {
        "--help",
        "-h",
        FCTCL_STORE_TRUE,
        "Shows this message"
    },
    {
        "--output",
        NULL,
        FCTCL_STORE_VALUE,
        "Name of file to store output."
    },
    FCTCL_INIT_NULL /* Sentinel. */
};


FCT_BGN()
{
    FCT_FIXTURE_SUITE_BGN(clp__parse_scenarios)
    {
        FCT_SETUP_BGN()
        {
            fct_clp__init(&clp, options);
        }
        FCT_SETUP_END();

        FCT_TEARDOWN_BGN()
        {
            fct_clp__final(&clp);
        }
        FCT_TEARDOWN_END();

        FCT_TEST_BGN(initialization)
        {
            fct_chk_eq_int( fct_clp__num_clo(&clp), 2);
        }
        FCT_TEST_END();

        FCT_TEST_BGN(parse_nothing)
        {
            char const *test_argv[] = {"program.exe" };
            int test_argc =1;

            fct_clp__parse(&clp, test_argc, test_argv);

            fct_chk( !fct_clp__is(&clp, "--help") );
            fct_chk( !fct_clp__is(&clp, "--output") );
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse_store_true)
        {
            char const *test_argv[] = {"program.exe", "--help"};
            int test_argc =2;

            fct_clp__parse(&clp, test_argc, test_argv);

            fct_chk( fct_clp__is(&clp, "--help") );
            fct_chk( !fct_clp__is(&clp, "--output") );
        }
        FCT_TEST_END();

        FCT_TEST_BGN(parse_store_true__short_arg)
        {
            char const *test_argv[] = {"program.exe", "-h"};
            int test_argc =2;

            fct_clp__parse(&clp, test_argc, test_argv);

            fct_chk( fct_clp__is(&clp, "--help") );
            fct_chk( fct_clp__is(&clp, "-h") );
            fct_chk( !fct_clp__is(&clp, "--output") );
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse_store_value__with_2nd_arg)
        {
            char const *test_argv[] = {"program.exe", "--output", "foo"};
            int test_argc =3;

            fct_clp__parse(&clp, test_argc, test_argv);

            fct_chk( !fct_clp__is(&clp, "--help") );
            fct_chk( fct_clp__is(&clp, "--output") );
            fct_chk_eq_str(fct_clp__optval(&clp, "--output"), "foo");
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse_store_value__with_equals)
        {
            char const *test_argv[] = {"program.exe", "--output=foo"};
            int test_argc =2;

            fct_clp__parse(&clp, test_argc, test_argv);

            fct_chk( !fct_clp__is(&clp, "--help") );
            fct_chk( fct_clp__is(&clp, "--output") );
            fct_chk_eq_str(fct_clp__optval(&clp, "--output"), "foo");
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse_store_value__with_equals_but_no_value)
        {
            char const *test_argv[] = {"program.exe", "--output="};
            int test_argc =2;

            fct_clp__parse(&clp, test_argc, test_argv);
            fct_chk( fct_clp__is_error(&clp) );

            fct_chk( !fct_clp__is(&clp, "--help") );
            fct_chk( !fct_clp__is(&clp, "--output") );
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse_store_value__without_2nd_arg)
        {
            char const *test_argv[] = {"program.exe", "--output"};
            int test_argc =2;

            fct_clp__parse(&clp, test_argc, test_argv);
            fct_chk( fct_clp__is_error(&clp) );

            fct_chk( !fct_clp__is(&clp, "--help") );
            fct_chk( !fct_clp__is(&clp, "--output") );
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse__check_for_invalid_value)
        {
            char const *test_argv[] = {"program.exe", "--output"};
            int test_argc =2;

            fct_clp__parse(&clp, test_argc, test_argv);

            fct_chk( !fct_clp__is(&clp, "--booga") );
            fct_chk( fct_clp__optval(&clp, "--booga") == NULL );
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse_store_value__with_multiple_args)
        {
            char const *test_argv[] = {"program.exe", "--output", "foo", "--help"};
            int test_argc =4;

            fct_clp__parse(&clp, test_argc, test_argv);

            fct_chk( fct_clp__is(&clp, "--help") );
            fct_chk( fct_clp__is(&clp, "--output") );
            fct_chk_eq_str(fct_clp__optval(&clp, "--output"), "foo");
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse_store_value__with_multiple_args_diff_order)
        {
            char const *test_argv[] = {"program.exe",  "--help", "--output", "xxx"};
            int test_argc =4;

            fct_clp__parse(&clp, test_argc, test_argv);

            fct_chk( fct_clp__is(&clp, "--help") );
            fct_chk( fct_clp__is(&clp, "--output") );
            fct_chk_eq_str(fct_clp__optval(&clp, "--output"), "xxx");
        }
        FCT_TEST_END();

        FCT_TEST_BGN(parse_store_value__with_params_only)
        {
            char const *test_argv[] = {"program.exe",
                                       "parama",
                                       "paramb",
                                       "paramc"
                                      };
            int test_argc =4;
            int is_error =0;
            int param_cnt =0;

            fct_clp__parse(&clp, test_argc, test_argv);
            is_error = fct_clp__is_error(&clp);
            fct_chk( !is_error );
            param_cnt = fct_clp__param_cnt(&clp);
            fct_chk_eq_int( param_cnt, 3);

            fct_chk( fct_clp__is_param(&clp, "parama") );
            fct_chk( !fct_clp__is_param(&clp, "funk") );
            fct_chk( fct_clp__is_param(&clp, "paramb") );
            fct_chk( fct_clp__is_param(&clp, "paramc") );
        }
        FCT_TEST_END();


        FCT_TEST_BGN(parse_store_value__with_params_and_a_flag)
        {
            char const *test_argv[] = {"program.exe",
                                       "--output=foo",
                                       "parama",
                                       "paramb",
                                       "paramc"
                                      };
            int test_argc =5;
            char const *optval;
            char const *paramat;

            fct_clp__parse(&clp, test_argc, test_argv);
            fct_chk( !fct_clp__is_error(&clp) );
            fct_chk_eq_int( fct_clp__param_cnt(&clp), 3);

            fct_chk( fct_clp__is_param(&clp, "parama") );
            fct_chk( !fct_clp__is_param(&clp, "funk") );
            fct_chk( fct_clp__is_param(&clp, "paramb") );
            fct_chk( fct_clp__is_param(&clp, "paramc") );

            /* Parameters should be in same sequence. Not neccessarily
            going to enforce this, just using the assumption for testing. */
            paramat = fct_clp__param_at(&clp, 0);
            fct_chk_eq_str( paramat, "parama");
            paramat = fct_clp__param_at(&clp, 1);
            fct_chk_eq_str( paramat, "paramb");
            paramat = fct_clp__param_at(&clp, 2);
            fct_chk_eq_str( paramat, "paramc");

            fct_chk( fct_clp__is(&clp, "--output") );
            optval = fct_clp__optval(&clp, "--output");
            fct_chk_eq_str( optval, "foo" );
        }
        FCT_TEST_END();
    }
    FCT_FIXTURE_SUITE_END();


}
FCT_END();
