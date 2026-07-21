/*
=========================================================================================================================
 *** iq_imbalance.cpp ***
 Implementation of the imbalance model, the blind moment estimator, the exact compensator, and the IRR measurement.
=========================================================================================================================

 Description:
     This file implements the interfaces declared in iq_imbalance.h. The model applies the impairment sample by
     sample. The estimator computes the rail means for the DC offsets, then the central second-order moments of the
     de-meaned rails: the gain estimate is sqrt(E[yQ^2] / E[yI^2]) and the phase estimate is
     asin(E[yI yQ] / sqrt(E[yI^2] E[yQ^2])), which are exact in expectation for a proper (circular) transmit signal
     because its rails are equal-power and uncorrelated. The compensator solves the two-by-two model for the original
     rails: I = yI - dcI and Q = ((yQ - dcQ) / g - sin(phi) I) / cos(phi), an exact algebraic inverse.

     The image rejection ratio is measured by correlating the signal against complex exponentials at the tone and
     image frequencies, which is a two-bin discrete Fourier transform evaluated directly; a full FFT is unnecessary
     for two bins and would add a dependency.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned as value types.

 Supporting files:
     header      include/iq_imbalance.h
=========================================================================================================================
*/
#include "iq_imbalance.h"

#include <cmath>

namespace iq {

Signal apply_imbalance(const Signal& x, const ImbalanceParams& p)
{
    Signal y;
    y.reserve(x.size());
    const double s = std::sin(p.phase);
    const double c = std::cos(p.phase);
    for (const cplx& v : x) {
        const double i = v.real();
        const double q = v.imag();
        // Standard receiver model: the I rail is the reference; the Q rail
        // carries the relative gain error and the quadrature phase error.
        y.emplace_back(i + p.dc_i,
                       p.gain * (s * i + c * q) + p.dc_q);
    }
    return y;
}

ImbalanceParams estimate(const Signal& y)
{
    const double n = static_cast<double>(y.size());

    double mi = 0.0, mq = 0.0;
    for (const cplx& v : y) { mi += v.real(); mq += v.imag(); }
    mi /= n; mq /= n;

    // Central moments of the de-meaned rails.
    double pii = 0.0, pqq = 0.0, piq = 0.0;
    for (const cplx& v : y) {
        const double i = v.real() - mi;
        const double q = v.imag() - mq;
        pii += i * i; pqq += q * q; piq += i * q;
    }
    pii /= n; pqq /= n; piq /= n;

    ImbalanceParams p;
    p.dc_i = mi;
    p.dc_q = mq;
    // For a proper transmit signal E[I^2] = E[Q^2] and E[IQ] = 0, so the
    // received moments isolate the impairment: the power ratio gives the
    // gain and the normalized cross-correlation gives sin(phase).
    p.gain  = std::sqrt(pqq / pii);
    p.phase = std::asin(piq / std::sqrt(pii * pqq));
    return p;
}

Signal compensate(const Signal& y, const ImbalanceParams& p)
{
    Signal x;
    x.reserve(y.size());
    const double s = std::sin(p.phase);
    const double c = std::cos(p.phase);
    for (const cplx& v : y) {
        const double i = v.real() - p.dc_i;
        // Exact algebraic inverse of the model, solved for the Q rail.
        const double q = ((v.imag() - p.dc_q) / p.gain - s * i) / c;
        x.emplace_back(i, q);
    }
    return x;
}

double bin_power(const Signal& x, double f0)
{
    // Direct two-sided correlation: X(f0) = sum x[n] e^{-j 2 pi f0 n} / N.
    cplx acc(0.0, 0.0);
    const double w = -2.0 * M_PI * f0;
    for (std::size_t nidx = 0; nidx < x.size(); ++nidx) {
        const double ph = w * static_cast<double>(nidx);
        acc += x[nidx] * cplx(std::cos(ph), std::sin(ph));
    }
    acc /= static_cast<double>(x.size());
    return std::norm(acc);
}

double irr_db(const Signal& x, double f0)
{
    const double sig = bin_power(x, f0);
    const double img = bin_power(x, -f0);
    return 10.0 * std::log10(sig / img);
}

}  // namespace iq
