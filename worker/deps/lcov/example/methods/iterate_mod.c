/*
 *  methods/iterate_mod.c
 *  
 *  identical to 'iterate.c', but with some trivial code changs to create
 *  differences - for differential coverage report.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "iterate.h"


int iterate_get_sum (int min, int max)
{
        int total = 0;

        /* This is where we loop over each number in the range, including
           both the minimum and the maximum number. */

        /* just reverse iteration order */
        for (int i = max; i >= min; total += i , --i)
        {
                /* We can detect an overflow by checking whether the new
                   sum would exceed the maximum integer value. */

                if (total > INT_MAX - i)
                {
                        printf ("Error: sum too large!\n");
                        exit (1);
                }

                /* Everything seems to fit into an int, so continue adding. */
        }

        return total;
}
