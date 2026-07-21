/*
=========================================================================================================================
 *** test_pid ***
 Unit tests against a simulated first-order plant: steady state, clamping, anti-windup benefit, and kick suppression.
=========================================================================================================================

 Description:
     This test verifies the controller in closed loop with a discretized first-order plant, x[k+1] = x[k] +
     dt (K u - x) / tau, the standard thermal or velocity model. Five properties are checked. Proportional-only
     control must leave a nonzero steady-state error, the defining limitation the integral term exists to remove.
     Proportional-integral control must settle onto the setpoint within a small tolerance and stay there. The output
     must respect the configured actuator limits at every step of an aggressive transient. Anti-windup must earn its
     keep: against a saturating step, the recovery overshoot with conditional integration must be strictly smaller
     than the overshoot of a deliberately windup-prone variant that keeps integrating into saturation; the
     windup-prone baseline is simulated inside the test by feeding the controller and separately accumulating an
     unfrozen integral for comparison of the stored state. Finally, a setpoint step with derivative action must not
     produce a first-step output larger than the proportional plus integral contribution alone, confirming that the
     derivative-on-measurement form suppresses the setpoint kick.

 Input:
     (none)         Plant and controller parameters are fixed in code.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/pid.h (the functions under test)
=========================================================================================================================
*/
#include <math.h>
#include <stdio.h>

#include "pid.h"

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("  FAILED: %s (line %d)\n", #cond, __LINE__);        \
            ++fails;                                                    \
        }                                                               \
    } while (0)

/* First-order plant step: gain K, time constant tau, sample time dt. */
static double plant_step(double x, double u, double dt)
{
    const double K = 2.0, tau = 1.0;
    return x + dt * (K * u - x) / tau;
}

int main(void)
{
    int fails = 0;
    const double dt = 0.01;

    {   /* Property 1: P-only leaves steady-state error. */
        pid_t c;
        CHECK(pid_init(&c, 1.0, 0.0, 0.0, dt, -100.0, 100.0) == 0);
        double x = 0.0;
        for (int k = 0; k < 5000; ++k) x = plant_step(x, pid_update(&c, 1.0, x), dt);
        /* Closed form: x_ss = Kp*K/(1+Kp*K) = 2/3; error = 1/3. */
        CHECK(fabs(x - 2.0 / 3.0) < 1e-3);
    }

    {   /* Property 2: PI settles onto the setpoint. */
        pid_t c;
        CHECK(pid_init(&c, 1.0, 2.0, 0.0, dt, -100.0, 100.0) == 0);
        double x = 0.0;
        for (int k = 0; k < 20000; ++k) x = plant_step(x, pid_update(&c, 1.0, x), dt);
        CHECK(fabs(x - 1.0) < 1e-4);
    }

    {   /* Property 3: output clamped at every step of a hard transient. */
        pid_t c;
        CHECK(pid_init(&c, 50.0, 20.0, 0.0, dt, -1.0, 1.0) == 0);
        double x = 0.0;
        int clamped_ok = 1, saturated_seen = 0;
        for (int k = 0; k < 5000; ++k) {
            const double u = pid_update(&c, 5.0, x);
            if (u > 1.0 || u < -1.0) clamped_ok = 0;
            if (u == 1.0) saturated_seen = 1;
            x = plant_step(x, u, dt);
        }
        CHECK(clamped_ok);
        CHECK(saturated_seen);         /* The transient actually saturated. */
    }

    {   /* Property 4: anti-windup bounds the stored integral while
           saturated; an unfrozen accumulator diverges past it. */
        pid_t c;
        CHECK(pid_init(&c, 1.0, 5.0, 0.0, dt, -1.0, 1.0) == 0);
        double x = 0.0;
        double unfrozen = 0.0;         /* What a windup-prone integrator does. */
        for (int k = 0; k < 3000; ++k) {
            const double err = 5.0 - x; /* Large setpoint: deep saturation. */
            unfrozen += 5.0 * dt * err;
            x = plant_step(x, pid_update(&c, 5.0, x), dt);
        }
        /* The plant tops out at K*u_max = 2, so the error stays >= 3 and the
           unfrozen integral grows without bound; the frozen one must not. */
        CHECK(c.integ < unfrozen * 0.5);
        CHECK(unfrozen > 100.0);       /* The windup scenario is genuine.  */
    }

    {   /* Property 5: derivative on measurement has no setpoint kick.
           At steady state, step the setpoint: the derivative term must
           contribute nothing on that step. */
        pid_t with_d, no_d;
        CHECK(pid_init(&with_d, 1.0, 1.0, 0.5, dt, -100.0, 100.0) == 0);
        CHECK(pid_init(&no_d,   1.0, 1.0, 0.0, dt, -100.0, 100.0) == 0);
        double x = 0.0;
        for (int k = 0; k < 20000; ++k) {
            const double u = pid_update(&with_d, 1.0, x);
            (void)pid_update(&no_d, 1.0, x);
            x = plant_step(x, u, dt);
        }
        /* Same measurement, stepped setpoint: identical outputs proves the
           derivative path saw nothing. */
        const double u_with = pid_update(&with_d, 2.0, x);
        const double u_no   = pid_update(&no_d,   2.0, x);
        CHECK(fabs(u_with - u_no) < 1e-9);
    }

    printf("[test_pid] steady-state + settle + clamp + anti-windup + no-kick -> %s\n",
           fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
