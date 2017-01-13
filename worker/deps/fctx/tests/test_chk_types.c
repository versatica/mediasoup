/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_chk_types.c

*/

#include <float.h>
#include <math.h>
#include "fct.h"

/* My Own Type. */
typedef struct _point_t
{
    float x,y,z;
} point_t;

#define point_is_eq(p1, p2, ep) \
    ((int)(fabs(p1.x - p2.x) < ep)) &&\
    ((int)(fabs(p1.y - p2.y) < ep)) &&\
    ((int)(fabs(p1.z - p2.z) < ep))

#define point_chk_eq(p1, p2, ep) \
    fct_xchk(\
        point_is_eq(p1, p2, ep), \
        "failed point_is_equal:\n<Point x=%f y=%f z=%f>"\
        " !=\n<Point x=%f y=%f z=%f>",\
        p1.x, p1.y, p1.z, p2.x, p2.y, p2.z\
        );

#define point_is_neq(p1, p2, ep) \
    (!(int)(fabs(p1.x - p2.x) < ep)) ||\
    (!(int)(fabs(p1.y - p2.y) < ep)) ||\
    (!(int)(fabs(p1.z - p2.z) < ep))

#define point_chk_neq(p1, p2, ep) \
    fct_xchk(\
        point_is_neq(p1, p2, ep), \
        "failed point_chk_neq:\n<Point x=%f y=%f z=%f>"\
        " ==\n<Point x=%f y=%f z=%f>",\
        p1.x, p1.y, p1.z, p2.x, p2.y, p2.z\
        );


FCT_BGN()
{
    FCT_QTEST_BGN(chk_dbl_eq)
    {
        fct_chk_eq_dbl(6123.2313,6123.2313);
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(chk_dbl_neq)
    {
        fct_chk_neq_dbl(1.11111, 1.1);
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(chk_str_eq)
    {
        fct_chk_eq_str("a", "a");
        fct_chk_eq_str(NULL, NULL);
        fct_chk_eq_str("boo", "boo");
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(chk_neq_str)
    {
        fct_chk_neq_str("a", "b");
        fct_chk_neq_str(NULL, "b");
        fct_chk_neq_str("a", NULL);
        fct_chk_neq_istr("different", "differentlengths");
        fct_chk_neq_istr("differentlengths", "different");
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(chk_eq_istr)
    {
        fct_chk_eq_istr("mismatch", "misMatch");
        fct_chk_eq_istr("a", "a");
        fct_chk_eq_istr("A", "a");
        fct_chk_eq_istr(NULL, NULL);
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(chk_neq_istr)
    {
        fct_chk_neq_istr("mismatch", "misMatchLength");
        fct_chk_neq_istr("misMatchLength", "mismatch");
        fct_chk_neq_istr("a", "b");
        fct_chk_neq_istr("A", "b");
        fct_chk_neq_istr(NULL, "b");
        fct_chk_neq_istr("A", NULL);
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_incl_str)
    {
        fct_chk_incl_str("mr roboto is super", "roboto");
        fct_chk_incl_str("mr ROBOTO is super", "ROBOTO");
        fct_chk_incl_str(NULL, NULL); /* NULL includes NULL */
        fct_chk_incl_str("mr roboto", NULL);
        fct_chk_incl_str("a", "a");
        fct_chk_incl_str("b", NULL);
        fct_chk_incl_str("mr roboto", "");
        fct_chk_incl_str("", "");
        fct_chk_incl_str("", NULL);
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_incl_istr)
    {
        /* All the case sensitive tests should pass. */
        fct_chk_incl_istr("mr roboto is super", "roboto");
        fct_chk_incl_istr("mr ROBOTO is super", "ROBOTO");
        fct_chk_incl_istr(NULL, NULL); /* NULL includes NULL */
        fct_chk_incl_istr("mr roboto", NULL);
        fct_chk_incl_istr("a", "a");
        fct_chk_incl_istr("b", NULL);
        fct_chk_incl_istr("mr roboto", "");
        fct_chk_incl_istr("", "");
        fct_chk_incl_istr("", NULL);
        fct_chk_incl_istr("MR RoboTO", "RoBOtO");
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_excl_str)
    {
        fct_chk_excl_str("mr roboto", "ta");
        fct_chk_excl_str("a", "b");
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_excl_istr)
    {
        fct_chk_excl_istr("mr ROboto", "ta");
        fct_chk_excl_istr("a", "b");
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_startswith_str)
    {
        fct_chk_startswith_str("mr ROboto", "mr");
        fct_chk_startswith_str(NULL, NULL);
        fct_chk_startswith_str("", "");
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_startswith_istr)
    {
        fct_chk_startswith_istr("Mr ROboto", "mr");
        fct_chk_startswith_istr(NULL, NULL);
        fct_chk_startswith_istr("", "");
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_endswith_str)
    {
        fct_chk_endswith_str("Mr ROboto", "ROboto");
        fct_chk_endswith_str(NULL, NULL);
        fct_chk_endswith_str("", "");
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_iendswith_str)
    {
        fct_chk_iendswith_str("Mr ROboto", "roboto");
        fct_chk_iendswith_str(NULL, NULL);
        fct_chk_iendswith_str("", "");
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_eq_int)
    {
        fct_chk_eq_int(1, 1);
        fct_chk_eq_int(-1, -1);
        fct_chk_eq_int(0, 0);
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_str_empty)
    {
        fct_chk_empty_str("");
        fct_chk_empty_str(NULL);
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_str_full)
    {
        fct_chk_full_str(" ");
        fct_chk_full_str("mr roboto");
    }
    FCT_QTEST_END();


    /* ---------------------------------------------------------- */
    FCT_QTEST_BGN(chk_neq_int)
    {
        fct_chk_neq_int(1, 2);
        fct_chk_neq_int(0, -1);
        fct_chk_neq_int(-1, 2);
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(chk_my_point)
    {
        point_t point1 = {1.f, 2.f, 3.f};
        point_t point2 = {1.f, 2.f, 3.f};
        point_t point3 = {10.f, 20.f, 30.f};
        point_chk_eq(point1, point2, DBL_EPSILON);
        point_chk_neq(point1, point3, DBL_EPSILON);
    }
    FCT_QTEST_END();
}
FCT_END();
