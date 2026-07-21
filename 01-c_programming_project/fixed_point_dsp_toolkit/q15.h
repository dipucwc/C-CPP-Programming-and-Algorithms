/*
=========================================================================================================================
 *** q15.h ***
 Q15 fixed-point arithmetic primitives with saturation, for processors without a floating-point unit.
=========================================================================================================================

 Description:
     This header provides the Q15 fixed-point type and its basic operations. Q15 stores a real number in the range
     -1.0 to just under +1.0 inside a signed 16-bit integer, with 15 fractional bits, so the resolution is 2^-15.
     This is the standard sample format of fixed-point audio and baseband processing. All operations saturate instead
     of wrapping: a sum that exceeds the representable range clips to the maximum or minimum value, which is the
     behaviour required in signal chains, where wrap-around would turn a small overflow into a full-scale glitch.

     Multiplication of two Q15 values produces a Q30 intermediate in a 32-bit accumulator; the result is renormalized
     to Q15 with a rounding constant added before the shift, which halves the bias compared to truncation. The
     conversion helpers to and from double are intended for test benches and host-side verification only, not for the
     processing path.

 Input:
     (none)         Header-only interfaces.

 Output:
     (none)         Header-only interfaces.

 Supporting files:
     tests       tests/test_q15.c (saturation, rounding, and round-trip accuracy)
=========================================================================================================================
*/
#ifndef Q15_H
#define Q15_H

#include <stdint.h>

typedef int16_t q15_t;

#define Q15_ONE   ((q15_t)32767)    /* Largest representable value, ~0.99997. */
#define Q15_MIN   ((q15_t)-32768)
#define Q15_SHIFT 15

/* Saturate a 32-bit intermediate into the Q15 range. */
static inline q15_t q15_sat(int32_t x)
{
    if (x > Q15_ONE) return Q15_ONE;
    if (x < Q15_MIN) return Q15_MIN;
    return (q15_t)x;
}

static inline q15_t q15_add(q15_t a, q15_t b)
{
    /* Widen before adding: int16 + int16 can overflow int16, and signed
       overflow is undefined behaviour in C. The int32 sum is always exact. */
    return q15_sat((int32_t)a + (int32_t)b);
}

static inline q15_t q15_sub(q15_t a, q15_t b)
{
    return q15_sat((int32_t)a - (int32_t)b);
}

static inline q15_t q15_mul(q15_t a, q15_t b)
{
    /* Q15 x Q15 = Q30. Add half an LSB (1 << 14) before shifting back so the
       renormalization rounds to nearest instead of truncating toward zero. */
    int32_t p = (int32_t)a * (int32_t)b;
    return q15_sat((p + (1 << (Q15_SHIFT - 1))) >> Q15_SHIFT);
}

/* Host-side helpers for tests and verification; not for the processing path. */
static inline q15_t q15_from_double(double d)
{
    if (d >= 1.0)  return Q15_ONE;
    if (d <= -1.0) return Q15_MIN;
    /* Rounding: +0.5 for positive, -0.5 for negative, then truncate. */
    double scaled = d * 32768.0;
    return (q15_t)(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
}

static inline double q15_to_double(q15_t q)
{
    return (double)q / 32768.0;
}

#endif /* Q15_H */
