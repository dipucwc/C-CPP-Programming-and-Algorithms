/*
=========================================================================================================================
 *** main.c ***
 Demonstration: a noisy sine passes through the ring buffer into the moving average, and RMS is measured at both ends.
=========================================================================================================================

 Description:
     This program wires the three modules into one small processing chain, the way they would be used on a target. A
     sine wave with additive pseudo-random noise is generated in Q15, streamed through the ring buffer to model a
     producer-consumer sample path, filtered by a 16-tap moving average, and measured with the RMS routine before and
     after filtering. The moving average attenuates the wideband noise while passing the low-frequency sine, so the
     output RMS is expected to land close to the clean-sine RMS of amplitude/sqrt(2), visibly below the noisy input
     RMS. The demo prints the three RMS figures so the effect can be read directly from the output.

     The noise is generated with a small linear congruential generator kept inside this file, seeded with a fixed
     constant, so the demo output is reproducible from run to run.

 Input:
     (none)         The waveform parameters are set in code.

 Output:
     return value   Zero on normal completion.
     stdout         Three RMS figures: clean sine, noisy input, filtered output.

 Supporting files:
     header      include/q15.h, include/ring_buffer.h, include/dsp_filters.h
=========================================================================================================================
*/
#include <stdio.h>
#include <math.h>

/* M_PI is a POSIX extension, not part of strict C99, so it is defined here
   for portability under -std=c99 -Wpedantic. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "q15.h"
#include "ring_buffer.h"
#include "dsp_filters.h"

#define N_SAMPLES 2048
#define MA_TAPS   16

/* Small LCG for reproducible noise; constants from Numerical Recipes. */
static uint32_t lcg_state = 12345u;
static double noise_uniform(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return (double)lcg_state / 4294967296.0 - 0.5;   /* Uniform in [-0.5, 0.5). */
}

int main(void)
{
    static q15_t clean[N_SAMPLES], noisy[N_SAMPLES], filtered[N_SAMPLES];

    for (int i = 0; i < N_SAMPLES; ++i) {
        double s = 0.4 * sin(2.0 * M_PI * 8.0 * i / N_SAMPLES);   /* 8 cycles. */
        double n = 0.2 * noise_uniform();
        clean[i] = q15_from_double(s);
        noisy[i] = q15_from_double(s + n);
    }

    q15_t rb_storage[64];
    ring_buffer_t rb;
    rb_init(&rb, rb_storage, 64);

    q15_t ma_storage[MA_TAPS];
    moving_avg_t ma;
    ma_init(&ma, ma_storage, MA_TAPS);

    /* Producer-consumer chain: push into the buffer, pop into the filter. */
    for (int i = 0; i < N_SAMPLES; ++i) {
        (void)rb_push(&rb, noisy[i]);
        q15_t sample;
        if (rb_pop(&rb, &sample) == RB_OK) {
            filtered[i] = ma_update(&ma, sample);
        }
    }

    double rms_clean    = q15_to_double(dsp_rms(clean, N_SAMPLES));
    double rms_noisy    = q15_to_double(dsp_rms(noisy, N_SAMPLES));
    double rms_filtered = q15_to_double(dsp_rms(filtered, N_SAMPLES));

    printf("Fixed-point DSP chain demo (%d samples, %d-tap moving average)\n",
           N_SAMPLES, MA_TAPS);
    printf("  RMS clean sine      : %.4f  (theory A/sqrt(2) = %.4f)\n",
           rms_clean, 0.4 / sqrt(2.0));
    printf("  RMS noisy input     : %.4f\n", rms_noisy);
    printf("  RMS filtered output : %.4f\n", rms_filtered);
    printf("  noise reduced       : %s\n",
           (rms_filtered < rms_noisy) ? "yes" : "no");

    return 0;
}
