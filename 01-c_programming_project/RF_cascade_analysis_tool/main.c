/*
=========================================================================================================================
 *** main.c ***
 Demonstration: cascade budget of a four-stage receiver front end with sensitivity and dynamic range.
=========================================================================================================================

 Description:
     This program analyzes a representative direct-conversion receiver front end: a low-noise amplifier, a passive
     band-select filter, a mixer, and an intermediate-frequency amplifier, each entered with datasheet-style gain,
     noise figure, and output intercept numbers. The stage table is printed, the cascade gain, noise figure, and
     intercept points are computed and reported, and the receiver-level sensitivity and spurious-free dynamic range
     are evaluated for a one-megahertz channel with a ten-decibel required signal-to-noise ratio.

     The chain illustrates the two classic budget rules. The low-noise amplifier in front keeps the cascade noise
     figure close to its own, despite the lossy filter and mixer behind it. The output intercept is dominated by the
     final amplifier, because every earlier stage's contribution is amplified toward the output by the gain that
     follows it.

 Input:
     (none)         The chain and channel parameters are set in code.

 Output:
     return value   Zero on normal completion.
     stdout         The stage table, the cascade figures, the sensitivity, and the dynamic range.

 Supporting files:
     header      include/rf_cascade.h
=========================================================================================================================
*/
#include <stdio.h>

#include "rf_cascade.h"

int main(void)
{
    const rf_stage_t chain[] = {
        /*  name        gain    NF     OIP3  */
        { "LNA",        15.0,   1.2,   20.0 },
        { "BPF",        -2.0,   2.0,   50.0 },   /* Passive: NF equals loss. */
        { "Mixer",      -7.0,   7.5,   15.0 },
        { "IF amp",     20.0,   4.0,   25.0 },
    };
    const size_t n = sizeof chain / sizeof chain[0];

    const double bandwidth_hz = 1.0e6;
    const double snr_req_db   = 10.0;

    rf_cascade_t cas;
    if (rf_analyze(chain, n, &cas) != 0) {
        printf("analysis failed\n");
        return 1;
    }

    printf("RF receiver cascade budget\n\n");
    printf("  stage     gain(dB)   NF(dB)   OIP3(dBm)\n");
    for (size_t k = 0; k < n; ++k) {
        printf("  %-8s  %7.1f   %6.1f   %8.1f\n",
               chain[k].name, chain[k].gain_db,
               chain[k].nf_db, chain[k].oip3_dbm);
    }

    printf("\n  cascade gain  : %7.2f dB\n",  cas.gain_db);
    printf("  cascade NF    : %7.2f dB\n",  cas.nf_db);
    printf("  cascade OIP3  : %7.2f dBm\n", cas.oip3_dbm);
    printf("  cascade IIP3  : %7.2f dBm\n", cas.iip3_dbm);

    const double sens = rf_sensitivity_dbm(cas.nf_db, bandwidth_hz, snr_req_db);
    const double sfdr = rf_sfdr_db(cas.iip3_dbm, cas.nf_db, bandwidth_hz);

    printf("\n  channel bandwidth      : %.0f kHz\n", bandwidth_hz / 1e3);
    printf("  required SNR           : %.1f dB\n", snr_req_db);
    printf("  sensitivity            : %7.2f dBm\n", sens);
    printf("  spurious-free DR       : %7.2f dB\n", sfdr);

    return 0;
}
