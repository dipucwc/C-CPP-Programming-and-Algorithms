/*
=========================================================================================================================
 *** pid.c ***
 Implementation of the clamped, anti-windup, derivative-on-measurement PID update.
=========================================================================================================================

 Description:
     This file implements the controller declared in pid.h. The update computes the error, forms the proportional
     term, advances the integral accumulator, forms the derivative from the measurement difference, sums and clamps.
     The anti-windup is conditional integration: the accumulator is advanced first and rolled back if the summed
     output saturates in the same direction as the error, which freezes the integrator exactly while it cannot help
     and releases it the moment the output re-enters the linear range. The derivative uses the negated measurement
     difference, so a setpoint step contributes nothing through the derivative path.

     The first call after initialization is treated specially: with no previous measurement, the derivative term is
     suppressed for that single step instead of differentiating against an arbitrary initial value.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         The actuator command is returned by value.

 Supporting files:
     header      include/pid.h
=========================================================================================================================
*/
#include "pid.h"

int pid_init(pid_t *c, double kp, double ki, double kd,
             double dt, double out_min, double out_max)
{
    if (!c || dt <= 0.0 || out_min >= out_max) return -1;
    c->kp         = kp;
    c->ki_dt      = ki * dt;
    c->kd_over_dt = kd / dt;
    c->out_min    = out_min;
    c->out_max    = out_max;
    c->integ      = 0.0;
    c->prev_meas  = 0.0;
    c->primed     = 0;
    return 0;
}

double pid_update(pid_t *c, double setpoint, double measurement)
{
    const double err = setpoint - measurement;

    const double p = c->kp * err;

    /* Tentative integrator advance; may be rolled back below. */
    const double integ_new = c->integ + c->ki_dt * err;

    /* Derivative on measurement: a setpoint step does not pass through
       this term, so it produces no output spike ("derivative kick"). */
    double d = 0.0;
    if (c->primed) {
        d = -c->kd_over_dt * (measurement - c->prev_meas);
    }
    c->prev_meas = measurement;
    c->primed = 1;

    double out = p + integ_new + d;

    /* Conditional integration: keep the advance only when it does not
       push a saturated output further into saturation. Freezing (not
       clamping to a fixed band) lets the integrator carry whatever
       value the linear range requires, including large setpoints. */
    if (out > c->out_max) {
        out = c->out_max;
        if (err < 0.0) c->integ = integ_new;   /* Error now unwinds: allow. */
    } else if (out < c->out_min) {
        out = c->out_min;
        if (err > 0.0) c->integ = integ_new;
    } else {
        c->integ = integ_new;
    }

    return out;
}
