/*
=========================================================================================================================
 *** dsp_filters.h ***
 Streaming moving-average filter and block RMS measurement over Q15 samples.
=========================================================================================================================

 Description:
     This header declares two measurement and filtering blocks. The first is a streaming moving-average filter of
     configurable window length over caller-owned storage: each new sample updates a running sum by adding the new
     value and subtracting the oldest, so the per-sample cost is constant regardless of the window length. The sum is
     kept in a 32-bit accumulator, which cannot overflow for any window shorter than 65536 samples because each Q15
     sample contributes at most 2^15 in magnitude. The output is the sum divided by the window length with rounding.

     The second block computes the root-mean-square value of a sample block. The squares are accumulated in a 64-bit
     integer in Q30, the mean is taken, and the square root is computed with an integer Newton iteration, so the
     whole path stays in fixed point. RMS is the standard power measure of a signal and is the building block of
     signal-to-noise and level measurements.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/dsp_filters.c
     tests       tests/test_dsp_filters.c (constant input, step response, and RMS of known waveforms)
=========================================================================================================================
*/
#ifndef DSP_FILTERS_H
#define DSP_FILTERS_H

#include <stddef.h>
#include <stdint.h>
#include "q15.h"

typedef struct {
    q15_t  *window;     /* Caller-owned storage of 'length' samples. */
    size_t  length;
    size_t  pos;        /* Next slot to overwrite. */
    size_t  filled;     /* Samples seen so far, capped at 'length'. */
    int32_t sum;        /* Running sum of the window contents. */
} moving_avg_t;

int   ma_init(moving_avg_t *ma, q15_t *storage, size_t length);
q15_t ma_update(moving_avg_t *ma, q15_t sample);

/* RMS of a block of samples, computed entirely in fixed point. */
q15_t dsp_rms(const q15_t *samples, size_t n);

#endif /* DSP_FILTERS_H */
