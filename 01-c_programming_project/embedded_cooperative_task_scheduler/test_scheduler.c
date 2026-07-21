/*
=========================================================================================================================
 *** test_scheduler ***
 Unit tests: activation counts, phase stagger, priority order, enable control, and overrun accounting.
=========================================================================================================================

 Description:
     This test verifies the scheduler through five properties. The first property is rate correctness: over one
     thousand ticks served promptly, a task of period ten must run exactly one hundred times and a task of period
     twenty-five exactly forty times. The second property is phase stagger: two tasks sharing a period but registered
     with different offsets must never run on the same tick, which the test observes by recording the tick of every
     activation. The third property is priority: when two tasks are due on the same tick, the earlier-registered task
     must run first, observed through a shared sequence log. The fourth property is the enable switch: a disabled
     task must not run while disabled and must resume when re-enabled. The fifth property is overrun accounting: when
     many ticks arrive before the run function is called, a task must be dispatched once per serving pass with its
     missed activations counted as overruns, and the run counter plus the overrun counter must equal the activations
     an ideal loop would have delivered.

 Input:
     (none)         Tick sequences are generated in code.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/scheduler.h (the functions under test)
=========================================================================================================================
*/
#include <stdio.h>

#include "scheduler.h"

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("  FAILED: %s (line %d)\n", #cond, __LINE__);        \
            ++fails;                                                    \
        }                                                               \
    } while (0)

/* Shared instrumentation the task functions write into. */
static uint32_t g_seq[4096];
static uint32_t g_seq_len;
static scheduler_t *g_sched;

static void record_task(void *ctx)
{
    /* Log (task id, current served tick) as one packed word. */
    const uint32_t id = (uint32_t)(uintptr_t)ctx;
    if (g_seq_len < 4096) g_seq[g_seq_len++] = (id << 24) | (g_sched->served & 0xFFFFFF);
}

int main(void)
{
    int fails = 0;

    {   /* Properties 1-3: rates, stagger, priority over 1000 ticks. */
        scheduler_t s;
        sched_init(&s);
        g_sched = &s;
        g_seq_len = 0;

        const int tA = sched_add(&s, record_task, (void *)(uintptr_t)1, 10, 0);
        const int tB = sched_add(&s, record_task, (void *)(uintptr_t)2, 25, 0);
        const int tC = sched_add(&s, record_task, (void *)(uintptr_t)3, 10, 5);
        CHECK(tA == 0 && tB == 1 && tC == 2);

        for (int k = 0; k < 1000; ++k) {
            sched_tick(&s);
            (void)sched_run(&s);       /* Prompt serving: no overruns. */
        }

        CHECK(s.tasks[tA].runs == 100);
        CHECK(s.tasks[tB].runs == 40);
        CHECK(s.tasks[tC].runs == 100);
        CHECK(s.tasks[tA].overruns == 0);

        /* Stagger: A (offset 0) and C (offset 5) share period 10 but must
           never log the same tick. Priority: on ticks where A and B are
           both due (multiples of 50), A's log entry precedes B's. */
        int stagger_ok = 1, priority_ok = 1;
        for (uint32_t i = 0; i < g_seq_len; ++i) {
            const uint32_t id_i = g_seq[i] >> 24, tick_i = g_seq[i] & 0xFFFFFF;
            if (i + 1 < g_seq_len) {
                const uint32_t id_n = g_seq[i + 1] >> 24;
                const uint32_t tick_n = g_seq[i + 1] & 0xFFFFFF;
                if (tick_i == tick_n && id_i == 1 && id_n == 3) stagger_ok = 0;
                if (tick_i == tick_n && id_i == 2 && id_n == 1) priority_ok = 0;
            }
            if (id_i == 3 && tick_i % 10 != 5) stagger_ok = 0;
        }
        CHECK(stagger_ok);
        CHECK(priority_ok);
    }

    {   /* Property 4: enable switch. */
        scheduler_t s;
        sched_init(&s);
        g_sched = &s;
        const int t = sched_add(&s, record_task, (void *)(uintptr_t)1, 5, 0);

        for (int k = 0; k < 50; ++k) { sched_tick(&s); (void)sched_run(&s); }
        const uint32_t runs_before = s.tasks[t].runs;
        CHECK(runs_before == 10);

        sched_enable(&s, t, 0);
        for (int k = 0; k < 50; ++k) { sched_tick(&s); (void)sched_run(&s); }
        CHECK(s.tasks[t].runs == runs_before);

        sched_enable(&s, t, 1);
        for (int k = 0; k < 50; ++k) { sched_tick(&s); (void)sched_run(&s); }
        CHECK(s.tasks[t].runs > runs_before);
    }

    {   /* Property 5: overrun accounting when serving falls behind.
           100 ticks arrive unserved; a period-10 task then gets served.
           An ideal loop would have delivered 10 activations; the policy
           delivers 1 run + 9 counted overruns on the first serving pass. */
        scheduler_t s;
        sched_init(&s);
        g_sched = &s;
        const int t = sched_add(&s, record_task, (void *)(uintptr_t)1, 10, 0);

        for (int k = 0; k < 100; ++k) sched_tick(&s);
        (void)sched_run(&s);

        CHECK(s.tasks[t].runs + s.tasks[t].overruns == 10);
        CHECK(s.tasks[t].overruns == 9);

        /* After catch-up the schedule is realigned: 100 further ticks,
           promptly served, add exactly 10 clean runs. */
        for (int k = 0; k < 100; ++k) { sched_tick(&s); (void)sched_run(&s); }
        CHECK(s.tasks[t].overruns == 9);
        CHECK(s.tasks[t].runs == 1 + 10);
    }

    printf("[test_scheduler] rates + stagger + priority + enable + overrun -> %s\n",
           fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
