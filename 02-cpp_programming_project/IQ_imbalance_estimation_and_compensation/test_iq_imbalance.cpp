/*
=========================================================================================================================
 *** test_iq_imbalance ***
 Unit tests: exact inversion with true parameters, blind estimation accuracy, IRR improvement, and clean-signal safety.
=========================================================================================================================

 Description:
     This test verifies the module through four properties. The first property is exact inversion: compensating with
     the true impairment parameters must reconstruct the original signal to numerical precision, confirming that the
     compensator is the algebraic inverse of the model. The second property is estimation accuracy: on a proper
     random QPSK-like signal of four hundred thousand samples, the blindly estimated gain, phase, and DC offsets must
     match the configured truth within tight tolerances. The third property is the end goal: on a test tone, the
     image rejection ratio after blind compensation must improve by at least thirty decibels over the impaired
     signal. The fourth property is safety on clean signals: estimating on an unimpaired signal must return
     parameters close to identity, so applying compensation to a healthy receiver does no harm.

     The random signal uses a fixed-seed Mersenne Twister, so every run of the test is reproducible. The QPSK-like
     signal is proper by construction, satisfying the circularity assumption the estimator relies on.

 Input:
     (none)         All signals and parameters are generated in code with fixed seeds.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/iq_imbalance.h (the functions under test)
=========================================================================================================================
*/
#include <cmath>
#include <cstdio>
#include <random>

#include "iq_imbalance.h"

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("  FAILED: %s (line %d)\n", #cond, __LINE__);   \
            ++fails;                                                    \
        }                                                               \
    } while (0)

namespace {

// Proper QPSK-like signal: independent equiprobable +-1/sqrt(2) rails.
iq::Signal make_qpsk(std::size_t n, unsigned seed)
{
    std::mt19937 rng(seed);
    std::bernoulli_distribution bit(0.5);
    const double a = 1.0 / std::sqrt(2.0);
    iq::Signal x;
    x.reserve(n);
    for (std::size_t k = 0; k < n; ++k) {
        x.emplace_back(bit(rng) ? a : -a, bit(rng) ? a : -a);
    }
    return x;
}

iq::Signal make_tone(std::size_t n, double f0)
{
    iq::Signal x;
    x.reserve(n);
    for (std::size_t k = 0; k < n; ++k) {
        const double ph = 2.0 * M_PI * f0 * static_cast<double>(k);
        x.emplace_back(std::cos(ph), std::sin(ph));
    }
    return x;
}

iq::ImbalanceParams test_truth()
{
    iq::ImbalanceParams p;
    p.gain  = std::pow(10.0, 1.0 / 20.0);      // 1 dB.
    p.phase = 5.0 * M_PI / 180.0;              // 5 degrees.
    p.dc_i  = 0.01;
    p.dc_q  = -0.02;
    return p;
}

}  // namespace

int main()
{
    int fails = 0;

    {   // Property 1: compensating with the true parameters is exact.
        const auto x = make_qpsk(10000, 1);
        const auto y = iq::apply_imbalance(x, test_truth());
        const auto z = iq::compensate(y, test_truth());
        double max_err = 0.0;
        for (std::size_t k = 0; k < x.size(); ++k) {
            max_err = std::max(max_err, std::abs(z[k] - x[k]));
        }
        CHECK(max_err < 1e-12);
    }

    {   // Property 2: blind estimates match the truth on a long record.
        // 400k samples: the moment estimator's standard error on sin(phase)
        // is ~0.5/sqrt(N/4) ~ 1.6e-3, giving 3x margin inside the 5e-3 bound.
        const auto x = make_qpsk(400000, 2);
        const auto y = iq::apply_imbalance(x, test_truth());
        const auto est = iq::estimate(y);
        const auto truth = test_truth();
        CHECK(std::fabs(est.gain - truth.gain) < 5e-3);
        CHECK(std::fabs(est.phase - truth.phase) < 5e-3);
        CHECK(std::fabs(est.dc_i - truth.dc_i) < 5e-3);
        CHECK(std::fabs(est.dc_q - truth.dc_q) < 5e-3);
    }

    {   // Property 3: blind compensation improves tone IRR by >= 30 dB.
        const double f0 = 64.0 / 4096.0;
        const auto x = make_tone(4096, f0);
        const auto y = iq::apply_imbalance(x, test_truth());
        const auto est = iq::estimate(y);
        const auto z = iq::compensate(y, est);
        const double before = iq::irr_db(y, f0);
        const double after  = iq::irr_db(z, f0);
        CHECK(after - before >= 30.0);
        CHECK(before < 30.0);      // The impairment is visible to begin with.
    }

    {   // Property 4: estimating on a clean signal returns near-identity.
        const auto x = make_qpsk(100000, 3);
        const auto est = iq::estimate(x);
        CHECK(std::fabs(est.gain - 1.0) < 5e-3);
        CHECK(std::fabs(est.phase) < 5e-3);
        CHECK(std::fabs(est.dc_i) < 5e-3);
        CHECK(std::fabs(est.dc_q) < 5e-3);
    }

    std::printf("[test_iq_imbalance] inverse + estimation + irr + clean -> %s\n",
                fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
