/*
=========================================================================================================================
 *** scheduler.c ***
 Implementation of the tick-driven cooperative scheduler with catch-up dispatch and overrun counting.
=========================================================================================================================

 Description:
     This file implements the scheduler declared in scheduler.h. The tick function increments the volatile counter
     and nothing else, keeping the interrupt path minimal. The run function advances the served tick toward the
     current tick. The run function snapshots the tick counter and serves the whole pending batch as one instant, the
     latest completed tick: it walks the task table in registration order, dispatches every enabled task whose due
     tick has arrived exactly once, counts every further missed activation as an overrun, and realigns the task's next
     due tick forward past the missed slots. This is the conventional policy for rate-based firmware tasks: better
     one late execution and an honest counter than a burst of stale catch-up calls, which is what serving the backlog
     tick-by-tick would produce.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned through values and counters.

 Supporting files:
     header      include/scheduler.h
=========================================================================================================================
*/
#include "scheduler.h"

#include <string.h>

void sched_init(scheduler_t *s)
{
    memset(s, 0, sizeof *s);
}

int sched_add(scheduler_t *s, sched_fn_t fn, void *ctx,
              uint32_t period, uint32_t offset)
{
    if (!s || !fn || period == 0 || s->n_tasks >= SCHED_MAX_TASKS) return -1;
    sched_task_t *t = &s->tasks[s->n_tasks];
    t->fn = fn;
    t->ctx = ctx;
    t->period = period;
    t->next_due = offset;
    t->runs = 0;
    t->overruns = 0;
    t->enabled = 1;
    return (int)s->n_tasks++;
}

void sched_enable(scheduler_t *s, int id, int on)
{
    if (!s || id < 0 || (uint32_t)id >= s->n_tasks) return;
    s->tasks[id].enabled = (uint8_t)(on ? 1 : 0);
}

void sched_tick(scheduler_t *s)
{
    /* Interrupt context: increment only. Dispatch happens in the loop. */
    s->tick++;
}

uint32_t sched_run(scheduler_t *s)
{
    uint32_t dispatched = 0;

    /* Snapshot the ISR-written counter once per entry; it may advance
       underneath us, and the next call will serve whatever it adds. */
    const uint32_t now = s->tick;
    if (s->served >= now) return 0;

    /* Serve the whole pending batch as one instant: the latest completed
       tick. Serving tick-by-tick instead would replay history -- a task
       10 periods late would be called 10 times in a burst with stale
       phase, which is exactly the behaviour the overrun policy exists
       to prevent. */
    const uint32_t t_now = now - 1;

    for (uint32_t i = 0; i < s->n_tasks; ++i) {
        sched_task_t *t = &s->tasks[i];
        if (!t->enabled || t_now < t->next_due) continue;

        /* Count how far behind this task is; every full period of
           lateness is a missed activation, reported, not replayed. */
        const uint32_t late = t_now - t->next_due;
        const uint32_t missed = late / t->period;
        t->overruns += missed;

        t->fn(t->ctx);
        t->runs++;
        dispatched++;

        /* Realign forward past every missed slot. */
        t->next_due += (missed + 1) * t->period;
    }
    s->served = now;
    return dispatched;
}
