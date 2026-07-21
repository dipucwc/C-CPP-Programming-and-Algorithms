/*
=========================================================================================================================
 *** pid.h ***
 Discrete PID controller with output clamping, integrator anti-windup, and derivative on measurement.
=========================================================================================================================

 Description:
     This header declares a discrete-time PID controller in the form used in embedded control loops. Three practical
     refinements distinguish it from the textbook formula. The output is clamped to configured actuator limits,
     because real actuators saturate. The integrator is conditionally frozen while the output is saturated in the
     direction that would deepen the saturation, the anti-windup measure that prevents the integral term from
     accumulating error the actuator cannot act on and then dragging the response into long overshoot on recovery.
     The derivative acts on the measurement rather than on the error, which removes the output spike that a setpoint
     step would otherwise inject through the derivative term, a refinement known as derivative on measurement.

     The controller keeps its entire state in a caller-owned context structure, performs no allocation, and exposes
     a single update function called once per sample period with the setpoint and the measurement. The sample period
     is fixed at initialization and folded into the integral and derivative gains, so the update contains no division.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/pid.c
     tests       tests/test_pid.c (steady state, clamping, anti-windup bound, setpoint-kick suppression)
=========================================================================================================================
*/
#ifndef PID_H
#define PID_H

typedef struct {
    /* Configuration, folded with the sample time at init. */
    double kp;
    double ki_dt;        /* ki * dt: per-step integral gain.        */
    double kd_over_dt;   /* kd / dt: per-step derivative gain.      */
    double out_min;
    double out_max;

    /* State. */
    double integ;        /* Integral accumulator (in output units). */
    double prev_meas;    /* Previous measurement for the derivative. */
    int    primed;       /* First-call flag: no derivative yet.      */
} pid_t;

/* Initialize with gains, the fixed sample time dt, and actuator limits. */
int pid_init(pid_t *c, double kp, double ki, double kd,
             double dt, double out_min, double out_max);

/* One control step: returns the clamped actuator command. */
double pid_update(pid_t *c, double setpoint, double measurement);

#endif /* PID_H */
