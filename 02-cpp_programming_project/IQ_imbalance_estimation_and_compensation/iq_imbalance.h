/*
=========================================================================================================================
 *** iq_imbalance.h ***
 Receiver IQ imbalance model, blind parameter estimation from second-order statistics, and exact compensation.
=========================================================================================================================

 Description:
     This header declares the three stages of the IQ imbalance problem. The impairment model applies a direct-current
     offset to each rail and a gain and phase mismatch to the Q rail relative to the I rail, following the standard
     receiver model y_I = I + dc_I and y_Q = g (sin(phi) I + cos(phi) Q) + dc_Q. In a quadrature receiver this
     mismatch arises from component tolerances in the two analogue paths and creates an image of the signal mirrored
     in frequency, quantified by the image rejection ratio.

     The estimator recovers the three impairment parameters blindly, using only the received samples and the
     circularity of the ideal signal: for a proper complex signal the two rails have equal power and zero
     correlation, so the DC offsets are the rail means, the gain mismatch is the square root of the rail power ratio,
     and the sine of the phase mismatch is the normalized cross-correlation of the rails. The compensator inverts the
     model exactly with the estimated parameters, so the residual image is set by estimation accuracy, which improves
     with the number of samples.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/iq_imbalance.cpp
     tests       tests/test_iq_imbalance.cpp (estimation accuracy and exact-inverse properties)
     reference   Valkama, Renfors, Koivunen, "Advanced methods for I/Q imbalance compensation
                 in communication receivers," IEEE Trans. Signal Processing, 2001 (blind moment estimator)
=========================================================================================================================
*/
#ifndef IQ_IMBALANCE_H
#define IQ_IMBALANCE_H

#include <complex>
#include <cstddef>
#include <vector>

namespace iq {

using cplx = std::complex<double>;
using Signal = std::vector<cplx>;

struct ImbalanceParams {
    double gain     = 1.0;   // Q-rail gain relative to I-rail (linear).
    double phase    = 0.0;   // Quadrature phase error in radians.
    double dc_i     = 0.0;   // DC offset on the I rail.
    double dc_q     = 0.0;   // DC offset on the Q rail.
};

// Apply the receiver imbalance model to an ideal signal.
Signal apply_imbalance(const Signal& x, const ImbalanceParams& p);

// Blindly estimate the imbalance parameters from received samples only.
ImbalanceParams estimate(const Signal& y);

// Invert the model with the given (estimated) parameters.
Signal compensate(const Signal& y, const ImbalanceParams& p);

// Power of the DFT bin at normalized frequency f0 (cycles per sample),
// evaluated by direct correlation so no FFT library is needed.
double bin_power(const Signal& x, double f0);

// Image rejection ratio in dB for a test tone at +f0: signal-bin power
// over image-bin power at -f0. Higher is better; ideal hardware is infinite.
double irr_db(const Signal& x, double f0);

}  // namespace iq

#endif  // IQ_IMBALANCE_H
