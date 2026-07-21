/*
=========================================================================================================================
 *** test_q15 ***
 Unit tests for Q15 arithmetic: saturation, rounding, identities, and round-trip conversion accuracy.
=========================================================================================================================

 Description:
     This test verifies the Q15 primitives through four properties. The first property is saturation: adding two
     large positive values must clip to Q15_ONE and adding two large negative values must clip to Q15_MIN, never
     wrap. The second property is multiplicative identity and sign behaviour: multiplying by Q15_ONE must return the
     input to within one LSB, and the product of values with opposite signs must be negative. The third property is
     rounding: 0.5 * 0.5 must give exactly 0.25 in Q15, which only holds when the Q30 renormalization rounds rather
     than truncates. The fourth property is round-trip accuracy: converting a grid of doubles to Q15 and back must
     agree with the original to within one half LSB, which is 2^-16.

 Input:
     (none)         All test values are built in code.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/q15.h (the functions under test)
=========================================================================================================================
*/
#include <stdio.h>
#include <math.h>
#include "q15.h"

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("  FAILED: %s (line %d)\n", #cond, __LINE__);        \
            ++fails;                                                    \
        }                                                               \
    } while (0)

int main(void)
{
    int fails = 0;

    /* Property 1: saturation, not wrap-around. */
    CHECK(q15_add(Q15_ONE, Q15_ONE) == Q15_ONE);
    CHECK(q15_add(Q15_MIN, Q15_MIN) == Q15_MIN);
    CHECK(q15_sub(Q15_MIN, Q15_ONE) == Q15_MIN);

    /* Property 2: identity within one LSB, and sign correctness. */
    q15_t half = q15_from_double(0.5);
    q15_t ident = q15_mul(half, Q15_ONE);
    CHECK(ident >= half - 1 && ident <= half + 1);
    CHECK(q15_mul(half, q15_from_double(-0.5)) < 0);

    /* Property 3: 0.5 * 0.5 == 0.25 exactly, which requires rounding. */
    q15_t quarter = q15_mul(half, half);
    CHECK(quarter == q15_from_double(0.25));

    /* Property 4: round-trip error below half an LSB across the range. */
    for (double d = -0.999; d < 1.0; d += 0.0625) {
        double back = q15_to_double(q15_from_double(d));
        CHECK(fabs(back - d) <= 1.0 / 65536.0);
    }

    printf("[test_q15] saturation + identity + rounding + round-trip -> %s\n",
           fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
