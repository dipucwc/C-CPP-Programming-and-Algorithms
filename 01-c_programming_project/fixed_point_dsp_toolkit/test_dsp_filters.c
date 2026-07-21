/*
=========================================================================================================================
 *** test_dsp_filters ***
 Unit tests for the moving average and RMS: constant input, step response, sine RMS, and DC RMS.
=========================================================================================================================

 Description:
     This test verifies the two DSP blocks through four properties. The first property is the constant-input
     behaviour of the moving average: feeding a constant must return that constant from the first sample onward,
     which exercises the fill-in divisor. The second property is the step response: after a step from zero to a
     constant, the output must reach the constant exactly once the window is fully inside the step, and must be
     monotonically non-decreasing on the way. The third property is the RMS of a full-cycle sine wave, which must
     equal the amplitude divided by the square root of two to within a small fixed-point tolerance; the reference
     value is computed with double-precision floating point on the host. The fourth property is the RMS of a DC
     block, which must equal the DC level itself.

 Input:
     (none)         The waveforms are generated in code with host floating point.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/dsp_filters.h (the functions under test)
     reference   RMS of a sine of amplitude A over full cycles is A / sqrt(2)
=========================================================================================================================
*/
#include <stdio.h>
#include <math.h>

/* M_PI is a POSIX extension, not part of strict C99, so it is defined here
   for portability under -std=c99 -Wpedantic. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "dsp_filters.h"

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

    /* Property 1: constant input gives constant output from sample one. */
    {
        q15_t win[4];
        moving_avg_t ma;
        CHECK(ma_init(&ma, win, 4) == 0);
        q15_t c = q15_from_double(0.25);
        for (int i = 0; i < 10; ++i) {
            q15_t y = ma_update(&ma, c);
            CHECK(y >= c - 1 && y <= c + 1);
        }
    }

    /* Property 2: step response is monotone and settles to the step level. */
    {
        q15_t win[8];
        moving_avg_t ma;
        CHECK(ma_init(&ma, win, 8) == 0);
        for (int i = 0; i < 8; ++i) (void)ma_update(&ma, 0);

        q15_t step = q15_from_double(0.5);
        q15_t prev = 0, y = 0;
        for (int i = 0; i < 8; ++i) {
            y = ma_update(&ma, step);
            CHECK(y >= prev);
            prev = y;
        }
        CHECK(y >= step - 1 && y <= step + 1);
    }

    /* Property 3: RMS of a sine of amplitude 0.5 equals 0.5/sqrt(2). */
    {
        enum { N = 1024 };                 /* Full cycles: 8 periods of 128. */
        q15_t buf[N];
        for (int i = 0; i < N; ++i)
            buf[i] = q15_from_double(0.5 * sin(2.0 * M_PI * 8.0 * i / N));

        double rms = q15_to_double(dsp_rms(buf, N));
        double ref = 0.5 / sqrt(2.0);
        /* Tolerance covers Q15 quantization of the waveform plus the
           integer square root, both below 1e-3 at this amplitude. */
        CHECK(fabs(rms - ref) < 2e-3);
    }

    /* Property 4: RMS of a DC block equals the DC level. */
    {
        enum { N = 256 };
        q15_t buf[N];
        q15_t dc = q15_from_double(0.3);
        for (int i = 0; i < N; ++i) buf[i] = dc;
        double rms = q15_to_double(dsp_rms(buf, N));
        CHECK(fabs(rms - 0.3) < 1e-3);
    }

    printf("[test_dsp_filters] constant + step + sine-rms + dc-rms -> %s\n",
           fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
