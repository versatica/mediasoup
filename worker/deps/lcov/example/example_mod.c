 /*
 *  example_mod.c
 *
 * Identical behaviour to 'example.c' - but with some trivial code changes
 * (including this change to commment section) - to create a few differences,
 * for differential coverage report example.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "iterate.h"
#include "gauss.h"

static int start = 0;
static int end = 9;


int main (int argc, char* argv[])
{
        int total1, total2;

        /* Accept a pair of numbers as command line arguments. */

        if (argc == 3)
        {
                start   = atoi(argv[argc-2]);
                end     = atoi(argv[argc-1]);
        }


        /* Use both methods to calculate the result. */

        total1 = iterate_get_sum (start, end);
        total2 = gauss_get_sum (start, end);


        /* Make sure both results are the same. */

        if (total1 == total2)
        {

                printf ("Success, sum[%d..%d] = %d\n", start, end, total1);
                return 0;
        }
        printf ("Failure (%d != %d)!\n", total1, total2);
        return 1;
}
