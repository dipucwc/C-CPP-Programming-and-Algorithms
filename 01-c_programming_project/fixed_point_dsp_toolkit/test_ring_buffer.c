/*
=========================================================================================================================
 *** test_ring_buffer ***
 Unit tests for the circular buffer: fill and drain order, rejection at the limits, and wrap-around integrity.
=========================================================================================================================

 Description:
     This test verifies the ring buffer through four properties. The first property is order preservation: samples
     pushed in sequence must pop in the same sequence. The second property is rejection at the limits: pushing into a
     full buffer must return RB_FULL without disturbing the contents, and popping from an empty buffer must return
     RB_EMPTY. The third property is wrap-around integrity: after interleaved pushes and pops that force the indices
     past the end of the storage many times, the data must still come out in order, which exercises the modulo
     arithmetic. The fourth property is argument validation: null pointers and zero capacity must be reported as
     RB_BAD_ARG instead of being dereferenced.

 Input:
     (none)         All sequences are generated in code.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/ring_buffer.h (the functions under test)
=========================================================================================================================
*/
#include <stdio.h>
#include "ring_buffer.h"

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
    q15_t storage[8];
    ring_buffer_t rb;

    /* Property 4 first: argument validation. */
    CHECK(rb_init(NULL, storage, 8) == RB_BAD_ARG);
    CHECK(rb_init(&rb, NULL, 8) == RB_BAD_ARG);
    CHECK(rb_init(&rb, storage, 0) == RB_BAD_ARG);
    CHECK(rb_init(&rb, storage, 8) == RB_OK);

    /* Property 1: order preservation over a simple fill and drain. */
    for (int i = 0; i < 8; ++i) CHECK(rb_push(&rb, (q15_t)i) == RB_OK);
    CHECK(rb_is_full(&rb));

    /* Property 2: rejection at the limits. */
    CHECK(rb_push(&rb, 99) == RB_FULL);
    for (int i = 0; i < 8; ++i) {
        q15_t v;
        CHECK(rb_pop(&rb, &v) == RB_OK);
        CHECK(v == (q15_t)i);
    }
    q15_t dummy;
    CHECK(rb_pop(&rb, &dummy) == RB_EMPTY);
    CHECK(rb_is_empty(&rb));

    /* Property 3: wrap-around integrity under interleaved traffic. The
       indices cross the end of the 8-slot storage hundreds of times. */
    int next_in = 0, next_out = 0, ok = 1;
    for (int round = 0; round < 1000; ++round) {
        for (int k = 0; k < 3; ++k)
            if (rb_push(&rb, (q15_t)(next_in & 0x7FFF)) == RB_OK) ++next_in;
        for (int k = 0; k < 2; ++k) {
            q15_t v;
            if (rb_pop(&rb, &v) == RB_OK) {
                if (v != (q15_t)(next_out & 0x7FFF)) ok = 0;
                ++next_out;
            }
        }
    }
    CHECK(ok);
    CHECK(rb_count(&rb) == (size_t)(next_in - next_out));

    printf("[test_ring_buffer] order + limits + wrap-around + args -> %s\n",
           fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
