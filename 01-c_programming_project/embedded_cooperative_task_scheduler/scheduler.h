/*
=========================================================================================================================
 *** scheduler.h ***
 Tick-driven cooperative task scheduler with periods, phase offsets, priorities, and overrun accounting.
=========================================================================================================================

 Description:
     This header declares a cooperative scheduler of the kind that runs the main loop of non-RTOS firmware. Tasks are
     registered with a function, a context pointer, a period in ticks, and a phase offset that staggers tasks sharing
     a period so their execution does not pile onto the same tick. A hardware timer interrupt calls the tick function,
     which only increments a counter and is therefore safe at interrupt time; the main loop calls the run function,
     which dispatches every task that has become due, in registration order, so registration order is the priority
     order.

     The scheduler counts overruns per task: when the main loop falls behind and a task becomes due again before its
     previous due tick was served, the missed activations are counted rather than silently collapsed, which turns a
     latent real-time problem into a measurable number. All storage is caller-owned and fixed at compile time through
     the task table capacity, so the module performs no allocation.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/scheduler.c
     tests       tests/test_scheduler.c (activation counts, phase stagger, priority order, overrun accounting)
=========================================================================================================================
*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define SCHED_MAX_TASKS 8u

typedef void (*sched_fn_t)(void *ctx);

typedef struct {
    sched_fn_t fn;
    void      *ctx;
    uint32_t   period;      /* Ticks between activations.               */
    uint32_t   next_due;    /* Absolute tick of the next activation.    */
    uint32_t   runs;        /* Completed activations.                   */
    uint32_t   overruns;    /* Activations that were served late by a
                               full period or more.                     */
    uint8_t    enabled;
} sched_task_t;

typedef struct {
    sched_task_t   tasks[SCHED_MAX_TASKS];
    uint32_t       n_tasks;
    volatile uint32_t tick;   /* Written by the tick ISR, read by run. */
    uint32_t       served;    /* Tick value the run loop has processed. */
} scheduler_t;

void sched_init(scheduler_t *s);

/* Register a task; returns its id (>= 0) or -1 when the table is full.
   The first activation occurs at tick 'offset', then every 'period'. */
int  sched_add(scheduler_t *s, sched_fn_t fn, void *ctx,
               uint32_t period, uint32_t offset);

void sched_enable(scheduler_t *s, int id, int on);

/* Called from the timer interrupt: increment only, no dispatch. */
void sched_tick(scheduler_t *s);

/* Called from the main loop: dispatch everything due, in registration
   (priority) order. Returns the number of task activations dispatched. */
uint32_t sched_run(scheduler_t *s);

#endif /* SCHEDULER_H */
