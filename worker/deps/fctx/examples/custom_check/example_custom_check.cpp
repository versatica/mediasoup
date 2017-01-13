/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

This software is licensed as described in the file LICENSE, which
you should have received as part of this distribution.
====================================================================

File: test_1suite.c

Runs through 1 test suite. A very simple project for testing the layout
and execution of a test suite.

*/

#include <string>
#include <sstream>

#include "fct.h"

/*
--------------------------------------------------------------------
OUR TEST CLASS
--------------------------------------------------------------------
*/

/* A very simple and naive money class implementation. */
class money_t
{
    friend bool operator==(const money_t &a, const money_t &b);
public:
    money_t() : amount_(0), currency_(0) {};
    money_t(int amount, const std::string &currency)
            : amount_(amount), currency_(currency)
    {};
    virtual ~money_t() {};
    std::string  to_string() const
    {
        std::stringstream buf;
        buf << "<money_t amount: "
        << this->amount_
        << "; currency: "
        << this->currency_
        << ">";
        return buf.str();
    };
    void add(int amount)
    {
        this->amount_ += amount;
    };
private:
    int amount_;    /* Use integers to make compares simple for now. */
    std::string currency_;
};


bool operator==(const money_t &a, const money_t &b)
{
    return ((a.amount_ == b.amount_)
            && (b.currency_ == b.currency_));
}

/*
--------------------------------------------------------------------
CUSTOM CHECK
--------------------------------------------------------------------
*/

#define chk_money_eq(m1, m2) \
    fct_xchk(\
        m1 == m2,\
        "money_t not equal:\n%s != %s",\
        m1.to_string().c_str(),\
        m2.to_string().c_str()\
        )


/*
--------------------------------------------------------------------
UNIT TESTS
--------------------------------------------------------------------
*/

FCT_BGN()
{
    FCT_QTEST_BGN(test_money__no_helpful_info)
    {
        money_t m1(10, "USD");
        money_t m2(m1);
        fct_chk( m1 == m2 );
    }
    FCT_QTEST_END();

    FCT_QTEST_BGN(test_money__with_custom_check)
    {
        money_t m1(10, "USD");
        money_t m2(m1);
        chk_money_eq(m1, m2);
    }
    FCT_QTEST_END();
}
FCT_END();

