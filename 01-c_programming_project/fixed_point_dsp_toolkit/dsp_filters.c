/*
=========================================================================================================================
 *** dsp_filters.c ***
 Implementation of the streaming moving-average filter and the fixed-point RMS measurement.
=========================================================================================================================

 Description:
     This file implements the two blocks declared in dsp_filters.h. The moving average maintains a running sum: on
     every update the incoming sample is added and, once the window is full, the sample it overwrites is subtracted,
     giving constant-time updates. During the fill-in phase the divisor is the number of samples seen so far, so the
     output is meaningful from the first sample onward instead of ramping up from zero.

     The RMS routine accumulates sample squares in a 64-bit Q30 sum, divides by the count to get the mean square, and
     extracts the square root with an integer Newton iteration on the Q30 value, returning a Q15 result. The Newton
     iteration converges quadratically and is bounded to a fixed number of steps, so the routine has deterministic
     execution time, which matters in real-time processing paths.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned as values.

 Supporting files:
     header      include/dsp_filters.h
=========================================================================================================================
*/
#include "dsp_filters.h"

int ma_init(moving_avg_t *ma, q15_t *storage, size_t length)
{
    if (!ma || !storage || length == 0) return -1;
    ma->window = storage;
    ma->length = length;
    ma->pos    = 0;
    ma->filled = 0;
    ma->sum    = 0;
    for (size_t i = 0; i < length; ++i) storage[i] = 0;
    return 0;
}

q15_t ma_update(moving_avg_t *ma, q15_t sample)
{
    /* Running-sum update: O(1) per sample for any window length. The slot at
       'pos' holds either zero (fill-in phase) or the oldest sample (steady
       state), so subtracting it is correct in both phases. */
    ma->sum -= ma->window[ma->pos];
    ma->sum += sample;
    ma->window[ma->pos] = sample;
    ma->pos = (ma->pos + 1) % ma->length;

    if (ma->filled < ma->length) ma->filled++;

    /* Divide with rounding toward nearest; the sign split keeps the rounding
       symmetric for negative sums. */
    int32_t n = (int32_t)ma->filled;
    int32_t s = ma->sum;
    int32_t avg = (s >= 0) ? (s + n / 2) / n : (s - n / 2) / n;
    return q15_sat(avg);
}

/* isqrt64: integer square root of a 64-bit value by Newton iteration.
   Started from a power-of-two overestimate, the iteration converges in well
   under 32 steps; the loop is bounded so execution time is deterministic. */
static uint32_t isqrt64(uint64_t x)
{
    if (x == 0) return 0;

    /* Initial guess: 2^(ceil(bits/2)), guaranteed >= sqrt(x). */
    int bits = 0;
    for (uint64_t t = x; t > 0; t >>= 1) bits++;
    uint64_t g = (uint64_t)1 << ((bits + 1) / 2);

    for (int i = 0; i < 32; ++i) {
        uint64_t next = (g + x / g) / 2;
        if (next >= g) break;              /* Converged: sequence is decreasing. */
        g = next;
    }
    return (uint32_t)g;
}

q15_t dsp_rms(const q15_t *samples, size_t n)
{
    if (!samples || n == 0) return 0;

    /* Sum of squares in Q30. Worst case per sample is 2^30, so a 64-bit
       accumulator holds over 2^33 samples without overflow. */
    uint64_t acc = 0;
    for (size_t i = 0; i < n; ++i) {
        int32_t s = samples[i];
        acc += (uint64_t)(s * s);
    }

    uint64_t mean_sq_q30 = acc / n;

    /* sqrt(Q30) = Q15: the square root halves the exponent of the scale
       factor 2^30, landing exactly on the Q15 scale of 2^15. */
    uint32_t r = isqrt64(mean_sq_q30);
    return q15_sat((int32_t)r);
}
