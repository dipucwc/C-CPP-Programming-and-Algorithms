/*
=========================================================================================================================
 *** main.cpp ***
 Demonstration: a complex test tone through receiver IQ imbalance, blind estimation, compensation, and IRR verdicts.
=========================================================================================================================

 Description:
     This program demonstrates the full chain on a complex exponential test tone, the canonical IRR test signal. A
     tone at a positive normalized frequency is generated, the receiver imbalance model is applied with one decibel
     of gain error, five degrees of phase error, and small DC offsets on both rails, and the image rejection ratio is
     measured. The impairment parameters are then estimated blindly from the received samples, the compensator is
     applied, and the IRR is measured again. The printed report shows the true and estimated parameters side by side
     and the IRR before and after compensation, so the accuracy of the estimator and the depth of the image
     suppression can be read directly from the output.

     With one decibel of gain error and five degrees of phase error, the theoretical IRR of the impaired signal is
     near 26 dB; after compensation the residual image is set only by the finite-sample estimation error and the
     tone-on-bin measurement, so the improvement is large. The tone frequency is chosen to give an integer number of
     cycles over the record, keeping the two-bin measurement leakage-free.

 Input:
     (none)         The tone and impairment parameters are set in code.

 Output:
     return value   Zero on normal completion.
     stdout         True versus estimated parameters and the IRR before and after compensation.

 Supporting files:
     header      include/iq_imbalance.h
=========================================================================================================================
*/
#include <cmath>
#include <cstdio>

#include "iq_imbalance.h"

int main()
{
    constexpr std::size_t kSamples = 4096;
    constexpr double kToneFreq = 64.0 / kSamples;   // Integer cycles: leakage-free bins.

    // Complex exponential test tone at +kToneFreq, unit amplitude.
    iq::Signal tone;
    tone.reserve(kSamples);
    for (std::size_t n = 0; n < kSamples; ++n) {
        const double ph = 2.0 * M_PI * kToneFreq * static_cast<double>(n);
        tone.emplace_back(std::cos(ph), std::sin(ph));
    }

    // Impairment: 1 dB gain error, 5 degrees phase error, small DC offsets.
    iq::ImbalanceParams truth;
    truth.gain  = std::pow(10.0, 1.0 / 20.0);
    truth.phase = 5.0 * M_PI / 180.0;
    truth.dc_i  = 0.01;
    truth.dc_q  = -0.02;

    const iq::Signal rx = iq::apply_imbalance(tone, truth);
    const iq::ImbalanceParams est = iq::estimate(rx);
    const iq::Signal fixed = iq::compensate(rx, est);

    const double irr_before = iq::irr_db(rx, kToneFreq);
    const double irr_after  = iq::irr_db(fixed, kToneFreq);

    std::printf("IQ imbalance estimation and compensation demo (%zu samples)\n\n", kSamples);
    std::printf("  parameter        true        estimated\n");
    std::printf("  gain (linear)    %.6f    %.6f\n", truth.gain,  est.gain);
    std::printf("  phase (deg)      %.4f      %.4f\n",
                truth.phase * 180.0 / M_PI, est.phase * 180.0 / M_PI);
    std::printf("  dc offset I      %.6f    %.6f\n", truth.dc_i, est.dc_i);
    std::printf("  dc offset Q      %.6f   %.6f\n",  truth.dc_q, est.dc_q);
    std::printf("\n");
    std::printf("  IRR before compensation : %7.2f dB\n", irr_before);
    std::printf("  IRR after compensation  : %7.2f dB\n", irr_after);
    std::printf("  improvement             : %7.2f dB\n", irr_after - irr_before);

    return 0;
}
