/*
=========================================================================================================================
 *** ring_buffer.h ***
 Fixed-capacity circular buffer of Q15 samples with caller-owned storage, for streaming sample delivery.
=========================================================================================================================

 Description:
     This header declares a circular buffer that stores Q15 samples in a caller-provided array, so the module performs
     no dynamic allocation and is usable on bare-metal targets where malloc is unavailable or forbidden. The buffer
     tracks a head index for writing, a tail index for reading, and a count of stored samples; push fails when the
     buffer is full and pop fails when it is empty, so overflow policy stays in the hands of the caller instead of
     silently dropping data.

     The capacity is fixed at initialization and may be any positive size; the implementation uses the count field
     rather than the classic one-slot-empty trick, which keeps the full capacity usable and makes the full and empty
     conditions trivially distinct. All functions return a status code and take the buffer as an explicit handle, the
     standard pattern for reentrant C modules that must serve several independent streams.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/ring_buffer.c
     tests       tests/test_ring_buffer.c (fill, drain, wrap-around, and rejection cases)
=========================================================================================================================
*/
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include "q15.h"

typedef enum {
    RB_OK = 0,
    RB_FULL,
    RB_EMPTY,
    RB_BAD_ARG
} rb_status_t;

typedef struct {
    q15_t  *data;       /* Caller-owned storage. */
    size_t  capacity;
    size_t  head;       /* Next write position. */
    size_t  tail;       /* Next read position. */
    size_t  count;      /* Samples currently stored. */
} ring_buffer_t;

rb_status_t rb_init(ring_buffer_t *rb, q15_t *storage, size_t capacity);
rb_status_t rb_push(ring_buffer_t *rb, q15_t sample);
rb_status_t rb_pop(ring_buffer_t *rb, q15_t *out);
size_t      rb_count(const ring_buffer_t *rb);
int         rb_is_full(const ring_buffer_t *rb);
int         rb_is_empty(const ring_buffer_t *rb);

#endif /* RING_BUFFER_H */
