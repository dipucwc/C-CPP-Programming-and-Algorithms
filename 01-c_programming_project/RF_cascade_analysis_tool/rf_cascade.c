/*
=========================================================================================================================
 *** rf_cascade.c ***
 Implementation of the cascade gain, Friis noise figure, IP3 combination, sensitivity, and dynamic range.
=========================================================================================================================

 Description:
     This file implements the analysis declared in rf_cascade.h. All cascade mathematics is carried out in linear
     units and converted to decibels only at the interfaces, because the Friis and intercept formulas are sums of
     linear quantities and mixing scales inside a formula is the classic source of cascade-spreadsheet errors. The
     Friis accumulation walks the chain once, maintaining the running gain product that divides each stage's excess
     noise. The intercept accumulation walks the chain once in the same pass, maintaining for each stage the gain
     that follows it, which weights that stage's contribution at the chain output.

     The sensitivity builds the input noise floor from the thermal density, the bandwidth, and the cascade noise
     figure, then adds the required signal-to-noise ratio. The spurious-free dynamic range applies the standard
     two-thirds rule between the input-referred intercept and the same noise floor.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned through value types and output parameters.

 Supporting files:
     header      include/rf_cascade.h
=========================================================================================================================
*/
#include "rf_cascade.h"

#include <math.h>

double rf_db_to_lin(double db)
{
    return pow(10.0, db / 10.0);
}

double rf_lin_to_db(double lin)
{
    return 10.0 * log10(lin);
}

int rf_analyze(const rf_stage_t *stages, size_t n, rf_cascade_t *out)
{
    if (!stages || !out || n == 0) return -1;

    double gain_total_lin = 1.0;
    double f_total = 0.0;          /* Cascade noise factor (linear). */

    /* Friis pass: each stage's excess noise (f - 1) is divided by the
       gain in front of it, so the first stage enters at full weight. */
    for (size_t k = 0; k < n; ++k) {
        const double f = rf_db_to_lin(stages[k].nf_db);
        if (k == 0) {
            f_total = f;
        } else {
            f_total += (f - 1.0) / gain_total_lin;
        }
        gain_total_lin *= rf_db_to_lin(stages[k].gain_db);
    }

    /* IP3 pass: stage k's output intercept is referred to the chain
       output by the gain that FOLLOWS stage k; reciprocals then add.
       Computed with a suffix-gain walk from the last stage backward. */
    double inv_ip3_sum = 0.0;      /* 1/mW, output-referred. */
    double gain_after_lin = 1.0;
    for (size_t k = n; k-- > 0; ) {
        const double ip3_mw = pow(10.0, stages[k].oip3_dbm / 10.0);
        inv_ip3_sum += 1.0 / (ip3_mw * gain_after_lin);
        gain_after_lin *= rf_db_to_lin(stages[k].gain_db);
    }

    out->gain_db  = rf_lin_to_db(gain_total_lin);
    out->nf_db    = rf_lin_to_db(f_total);
    out->oip3_dbm = 10.0 * log10(1.0 / inv_ip3_sum);
    out->iip3_dbm = out->oip3_dbm - out->gain_db;
    return 0;
}

double rf_sensitivity_dbm(double nf_db, double bandwidth_hz, double snr_db)
{
    /* Input noise floor = thermal density + bandwidth + noise figure. */
    return RF_KTB_DBM_PER_HZ + 10.0 * log10(bandwidth_hz) + nf_db + snr_db;
}

double rf_sfdr_db(double iip3_dbm, double nf_db, double bandwidth_hz)
{
    const double floor_dbm =
        RF_KTB_DBM_PER_HZ + 10.0 * log10(bandwidth_hz) + nf_db;
    /* Two-thirds rule: third-order products grow 3 dB per input dB, so
       the spur-free span is two thirds of the intercept-to-floor gap. */
    return (2.0 / 3.0) * (iip3_dbm - floor_dbm);
}
