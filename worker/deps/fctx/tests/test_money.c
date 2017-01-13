/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================
File: test_money.c

Emulates the testing a "Money" class. This is an example of testing
with something that is a little more "Real World". At least it
corresponds to the oft written about class.

This is build from the "JUnit Test Infected: Programmers Love Writing
Tests" paper found at the following URL,
http://junit.sourceforge.net/doc/testinfected/testing.htm.
*/

/* Include the FCT Unit Test framework. */
#include "fct.h"

/*
-----------------------------------------------------------------------
MONEY
-----------------------------------------------------------------------

Defines a "Simple" money object.
*/

/* Any currency "nickname" large than 16 must be a preversion of a
nickname. */
#define MONEY_MAX_CURR_LEN 16

typedef struct _money_t money_t;
struct _money_t
{
    int amount;
    char currency[MONEY_MAX_CURR_LEN];
};

static void
money__del(money_t *self)
{
    if ( self == NULL )
    {
        return;
    }
    free(self);
}

static money_t *
money_new(int amount, char const *currency)
{
    money_t *self =NULL;

    self = (money_t*)calloc(1, sizeof(money_t));
    if ( self == NULL )
    {
        return NULL;
    }

    strcpy(self->currency, currency);
    self->currency[MONEY_MAX_CURR_LEN-1] = '\0';

    self->amount = amount;

    return self;
}

static int
money__amount(money_t const *self)
{
    assert( self != NULL );
    return self->amount;
}

/* Returns a reference to the "currency string". Do NOT MODIFY! */
static char const *
money___currency(money_t const *self)
{
    assert( self != NULL );
    return self->currency;
}

/* Creates a copy of the existing money class. */
static money_t*
money__copy(money_t const *self)
{
    money_t *other = money_new(money__amount(self), money___currency(self));
    return other;
}

/* Can only add the same currency to the existing currency. If you
want to decrement, use a negative number for now. */
static void
money__add_amt(money_t *self, int amt)
{
    assert( self != NULL );
    self->amount = self->amount + amt;
}


/* Returns 1 if m1's currency is equal to m2's currency. */
static int
money_curr_eq(money_t const *m1, money_t const *m2)
{
    int is_eq =0;

    /* Simple case */
    if ( m1 == m2 )
    {
        return 1;
    }
    /* If money1 XOR money2 are NULL, then we kick out. */
    else if ( (m1 == NULL && m2 != NULL) || (m1 != NULL && m2 == NULL) )
    {
        return 0;
    }

    is_eq =strcmp(money___currency(m1), money___currency(m2)) ==0;

    return is_eq;
}

/* Returns 1 if money1 == money2 */
static int
money_eq(money_t const *money1, money_t const *money2)
{
    int is_same_curr =0;
    int is_same_amt =0;


    /* Are they pointing to the same memory, then they are the same object.
    This also catches the case when they are both NULL. */
    if ( money1 == money2 )
    {
        return 1;
    }

    /* If money1 XOR money2 are NULL, then we kick out. */
    if (
        (money1 == NULL && money2 != NULL) \
        || (money1 != NULL && money2 == NULL)
    )
    {
        return 0;
    }

    /* At this point we shouldn't have any NULL money objects. */
    assert( money1 != NULL );
    assert( money2 != NULL );

    is_same_amt = money__amount(money1) == money__amount(money2);
    if ( !is_same_amt )
    {
        return 0;
    }

    is_same_curr = money_curr_eq(money1, money2);

    /* If they are the same currency, and the passed the eariler tests, then
    they are the same object. */
    return is_same_curr;
}

static money_t*
money_add(money_t const *money1, money_t const *money2)
{
    money_t *new_amt =NULL;

    assert( money1 != NULL );
    assert( money2 != NULL );

    /* For now we will ignore "different" currencies, and return an error. */
    if ( !money_curr_eq(money1, money2) )
    {
        return NULL;
    }

    new_amt = money_new(
                  money__amount(money1)+money__amount(money2),
                  money___currency(money1)
              );
    return new_amt;
}

/*
-----------------------------------------------------------------------
UNIT TESTS
-----------------------------------------------------------------------
*/

FCT_BGN()
{
    /* Illustrates a VERY SIMPLE unit test, without a fixture. */
    FCT_SUITE_BGN(money_simple)
    {
        FCT_TEST_BGN(money_new_del__basic)
        {
            money_t *m = money_new(10, "US");
            fct_chk(m != NULL);
            money__del(m);
        }
        FCT_TEST_END();

        FCT_TEST_BGN(money_del__with_null)
        {
            /* Supply NULL to money_del, and make sure it does nothing. */
            money__del(NULL);
        }
        FCT_TEST_END();

        FCT_TEST_BGN(money__copy__basic)
        {
            money_t *m = money_new(10, "USD");
            money_t *other =NULL;

            fct_chk( m != NULL );

            other = money__copy(m);
            fct_chk( other != NULL );

            fct_chk( money_eq(m, other) );

            money__del(other);
            money__del(m);
        }
        FCT_TEST_END();
    }
    FCT_SUITE_END();

    /* Creates a unit with a fixture, to help bootstrap common code between
    tests. */
    {
        /* These objects are common to each test. */
        money_t *m12CDN =NULL;
        money_t *m14CDN =NULL;
        money_t *m7USD =NULL;

        FCT_FIXTURE_SUITE_BGN(money_fixture)
        {
            /* Notice we have a setup and teardown sections in our fixture test
            suite. These are used to consturct and destruct our common data
            defined above. */
            FCT_SETUP_BGN()
            {
                /* Our common setup procedure. */
                m12CDN = money_new(12, "CDN");
                m14CDN = money_new(14, "CDN");
                m7USD = money_new(7, "USD");
            }
            FCT_SETUP_END();

            FCT_TEARDOWN_BGN()
            {
                /* Our common cleanup procedure. */
                money__del(m12CDN);
                m12CDN =NULL;
                money__del(m14CDN);
                m14CDN =NULL;
            }
            FCT_TEARDOWN_END();

            FCT_TEST_BGN(money__add_amt__simple)
            {
                money_t *expected = money_new(32, "CDN");

                money__add_amt(m14CDN, 18); /* Should be 32 now */
                fct_chk( money_eq(m14CDN, expected) );

                money__del(expected);
            }
            FCT_TEST_END();

            FCT_TEST_BGN(money_add__simple)
            {
                money_t *expected = money_new(26, "CDN");

                money_t *result = money_add(m12CDN, m14CDN);
                fct_chk( money_eq(result, expected) );

                money__del(result);
            }
            FCT_TEST_END();

            FCT_TEST_BGN(money_add__diff_currency)
            {
                money_t *result;

                result = money_add(m12CDN, m7USD);

                /* Currently not implmented so NULL is returned. We can change
                this test later. */
                fct_chk( result == NULL );

                money__del(result); /* For later */
            }
            FCT_TEST_END();

            FCT_TEST_BGN(money_eq__simple)
            {
                money_t *m12CDN_prime = money_new(12, "CDN");

                fct_chk( !money_eq(m12CDN, NULL) );
                fct_chk(  money_eq(m12CDN, m12CDN) );
                fct_chk(  money_eq(m12CDN, m12CDN_prime) );
                fct_chk( !money_eq(m12CDN, m14CDN) );

                money__del(m12CDN_prime);
            }
            FCT_TEST_END();

            FCT_TEST_BGN(money_curr_eq__simple)
            {
                fct_chk( !money_curr_eq(m12CDN, m7USD) );
                fct_chk(  money_curr_eq(m12CDN, m14CDN) );
                fct_chk( !money_curr_eq(NULL, m7USD) );
                fct_chk( !money_curr_eq(m7USD, NULL) );
                fct_chk( money_curr_eq(m12CDN, m12CDN) );
            }
            FCT_TEST_END();
        }
        FCT_FIXTURE_SUITE_END();
    }





}
FCT_END();
