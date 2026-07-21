/*
=========================================================================================================================
 *** rf_cascade.h ***
 Cascade analysis of an RF receiver chain: gain, Friis noise figure, IP3, sensitivity, and dynamic range.
=========================================================================================================================

 Description:
     This header declares the data model and the analysis functions for a chain of RF stages, each described by its
     power gain, noise figure, and output-referred third-order intercept point. The cascade gain is the sum of the
     stage gains in decibels. The cascade noise figure follows the Friis formula in linear terms: the noise factor of
     each stage is divided by the total gain preceding it, so early stages dominate the noise performance, which is
     why receivers lead with a low-noise amplifier. The cascade intercept point follows the standard output-referred
     combination: the reciprocal of the total output IP3 is the sum of each stage's reciprocal IP3 weighted by the
     gain following that stage, so late stages dominate the linearity, which is why linearity budgets concentrate on
     the last amplifier.

     On top of the three cascade quantities, two receiver-level figures are provided. The sensitivity is the minimum
     detectable input power for a required signal-to-noise ratio in a given bandwidth, built on the thermal noise
     density of -174 dBm per hertz at the standard reference temperature of 290 kelvin. The spurious-free dynamic
     range is the input power span between the sensitivity-limited floor and the level at which third-order products
     rise above that floor, computed from the input-referred intercept point and the noise floor.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/rf_cascade.c
     tests       tests/test_rf_cascade.c (textbook Friis values, attenuator identity, IP3 combination)
     reference   H. T. Friis, "Noise figures of radio receivers," Proc. IRE, 1944
=========================================================================================================================
*/
#ifndef RF_CASCADE_H
#define RF_CASCADE_H

#include <stddef.h>

/* One RF stage as specified on a datasheet. */
typedef struct {
    const char *name;
    double gain_db;     /* Power gain; negative for passive loss.     */
    double nf_db;       /* Noise figure; equals the loss for a
                           matched passive attenuator.                */
    double oip3_dbm;    /* Output-referred third-order intercept.     */
} rf_stage_t;

/* Cascade results over the full chain. */
typedef struct {
    double gain_db;
    double nf_db;
    double oip3_dbm;    /* Output-referred. */
    double iip3_dbm;    /* Input-referred: oip3 - gain.               */
} rf_cascade_t;

/* Thermal noise density at the 290 K reference temperature. */
#define RF_KTB_DBM_PER_HZ (-174.0)

/* Unit conversions. */
double rf_db_to_lin(double db);
double rf_lin_to_db(double lin);

/* Analyze a chain of 'n' stages; returns nonzero on bad arguments. */
int rf_analyze(const rf_stage_t *stages, size_t n, rf_cascade_t *out);

/* Minimum detectable input power (dBm) for the given cascade noise
   figure, bandwidth in hertz, and required output SNR in decibels. */
double rf_sensitivity_dbm(double nf_db, double bandwidth_hz, double snr_db);

/* Spurious-free dynamic range (dB) from the input-referred intercept
   and the input noise floor over the given bandwidth. */
double rf_sfdr_db(double iip3_dbm, double nf_db, double bandwidth_hz);

#endif /* RF_CASCADE_H */
