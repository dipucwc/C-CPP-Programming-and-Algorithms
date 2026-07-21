/*
=========================================================================================================================
 *** ring_buffer.c ***
 Implementation of the fixed-capacity circular buffer declared in ring_buffer.h.
=========================================================================================================================

 Description:
     This file implements the circular buffer using head and tail indices advanced modulo the capacity, with a count
     field distinguishing the full and empty states. Push writes at the head and advances it; pop reads at the tail
     and advances it; both reject the operation with a status code when the state does not allow it. Every function
     validates its pointer arguments, so a null handle is reported instead of dereferenced.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned as status codes and output parameters.

 Supporting files:
     header      include/ring_buffer.h
=========================================================================================================================
*/
#include "ring_buffer.h"

rb_status_t rb_init(ring_buffer_t *rb, q15_t *storage, size_t capacity)
{
    if (!rb || !storage || capacity == 0) return RB_BAD_ARG;
    rb->data     = storage;
    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->count    = 0;
    return RB_OK;
}

rb_status_t rb_push(ring_buffer_t *rb, q15_t sample)
{
    if (!rb) return RB_BAD_ARG;
    if (rb->count == rb->capacity) return RB_FULL;

    rb->data[rb->head] = sample;
    /* Modulo advance; capacity is not required to be a power of two. */
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count++;
    return RB_OK;
}

rb_status_t rb_pop(ring_buffer_t *rb, q15_t *out)
{
    if (!rb || !out) return RB_BAD_ARG;
    if (rb->count == 0) return RB_EMPTY;

    *out = rb->data[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;
    return RB_OK;
}

size_t rb_count(const ring_buffer_t *rb)
{
    return rb ? rb->count : 0;
}

int rb_is_full(const ring_buffer_t *rb)
{
    return rb ? (rb->count == rb->capacity) : 0;
}

int rb_is_empty(const ring_buffer_t *rb)
{
    return rb ? (rb->count == 0) : 1;
}
