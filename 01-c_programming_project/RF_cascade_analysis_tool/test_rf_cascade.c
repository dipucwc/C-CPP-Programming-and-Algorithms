/*
=========================================================================================================================
 *** test_rf_cascade ***
 Unit tests: single-stage identity, textbook Friis value, attenuator identity, IP3 combination, and level figures.
=========================================================================================================================

 Description:
     This test verifies the cascade analysis against values that can be checked by hand. The first property is the
     single-stage identity: a chain of one stage must return that stage's own figures exactly. The second property is
     the classic two-stage Friis example: a first stage of ten decibels gain and two decibels noise figure followed
     by a second stage of ten decibels noise figure must give a cascade noise figure of 3.953 decibels, the value
     obtained by hand from F = F1 + (F2 - 1) / G1. The third property is the matched attenuator identity: a passive
     stage whose noise figure equals its loss must produce a cascade noise figure equal to that loss. The fourth
     property is the two-stage intercept combination: two identical stages of ten decibels gain and twenty dBm output
     intercept must combine to 19.586 dBm at the output, the value obtained by hand from the reciprocal sum with the
     first stage weighted by the following gain. The fifth property checks the receiver-level figures: with a five
     decibel noise figure, one megahertz bandwidth, and ten decibel required signal-to-noise ratio, the sensitivity
     must be -99 dBm exactly, and the dynamic-range formula must reproduce the two-thirds rule on a hand-checked
     case.

 Input:
     (none)         All chains and expected values are fixed in code.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/rf_cascade.h (the functions under test)
=========================================================================================================================
*/
#include <math.h>
#include <stdio.h>

#include "rf_cascade.h"

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("  FAILED: %s (line %d)\n", #cond, __LINE__);        \
            ++fails;                                                    \
        }                                                               \
    } while (0)

#define NEAR(a, b, tol) (fabs((a) - (b)) < (tol))

int main(void)
{
    int fails = 0;
    rf_cascade_t cas;

    {   /* Property 1: single stage returns its own figures. */
        const rf_stage_t s[] = { { "amp", 12.0, 3.0, 22.0 } };
        CHECK(rf_analyze(s, 1, &cas) == 0);
        CHECK(NEAR(cas.gain_db, 12.0, 1e-9));
        CHECK(NEAR(cas.nf_db, 3.0, 1e-9));
        CHECK(NEAR(cas.oip3_dbm, 22.0, 1e-9));
        CHECK(NEAR(cas.iip3_dbm, 10.0, 1e-9));
    }

    {   /* Property 2: textbook two-stage Friis.
           F = 10^0.2 + (10^1.0 - 1)/10 = 1.58489 + 0.9 = 2.48489
           NF = 10*log10(2.48489) = 3.9531 dB. */
        const rf_stage_t s[] = {
            { "s1", 10.0, 2.0, 100.0 },     /* Huge OIP3: linearity inert. */
            { "s2", 10.0, 10.0, 100.0 },
        };
        CHECK(rf_analyze(s, 2, &cas) == 0);
        CHECK(NEAR(cas.nf_db, 3.9531, 1e-3));
        CHECK(NEAR(cas.gain_db, 20.0, 1e-9));
    }

    {   /* Property 3: matched attenuator, NF equals loss. */
        const rf_stage_t s[] = { { "att", -6.0, 6.0, 100.0 } };
        CHECK(rf_analyze(s, 1, &cas) == 0);
        CHECK(NEAR(cas.nf_db, 6.0, 1e-9));
        CHECK(NEAR(cas.gain_db, -6.0, 1e-9));
    }

    {   /* Property 4: two-stage IP3 combination by hand.
           Both stages G = 10 dB, OIP3 = 20 dBm = 100 mW.
           Stage 1 referred to output: 100 mW * 10 = 1000 mW.
           1/IP3 = 1/1000 + 1/100 = 0.011 -> 90.909 mW -> 19.586 dBm. */
        const rf_stage_t s[] = {
            { "s1", 10.0, 3.0, 20.0 },
            { "s2", 10.0, 3.0, 20.0 },
        };
        CHECK(rf_analyze(s, 2, &cas) == 0);
        CHECK(NEAR(cas.oip3_dbm, 19.586, 1e-3));
        CHECK(NEAR(cas.iip3_dbm, 19.586 - 20.0, 1e-3));
    }

    {   /* Property 5: sensitivity and SFDR on hand-checked cases.
           Sensitivity: -174 + 60 + 5 + 10 = -99 dBm exactly.
           SFDR: floor = -174 + 60 + 5 = -109 dBm; with IIP3 = -10 dBm,
           SFDR = (2/3) * (-10 - (-109)) = 66.0 dB. */
        CHECK(NEAR(rf_sensitivity_dbm(5.0, 1.0e6, 10.0), -99.0, 1e-9));
        CHECK(NEAR(rf_sfdr_db(-10.0, 5.0, 1.0e6), 66.0, 1e-9));
    }

    {   /* Argument validation. */
        const rf_stage_t s[] = { { "x", 0.0, 0.0, 0.0 } };
        CHECK(rf_analyze(NULL, 1, &cas) != 0);
        CHECK(rf_analyze(s, 0, &cas) != 0);
        CHECK(rf_analyze(s, 1, NULL) != 0);
    }

    printf("[test_rf_cascade] identity + friis + attenuator + ip3 + levels -> %s\n",
           fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
