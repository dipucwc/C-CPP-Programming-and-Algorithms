/*
=========================================================================================================================
 *** lottery.cpp ***
 Implementation of unbiased draw generation and Monte Carlo chi-square uniformity analysis.
=========================================================================================================================

 Description:
     This file implements the interfaces declared in lottery.h. The draw generator builds the candidate pool 1 to
     kPoolSize once per call and applies std::sample, which selects kPicks elements without replacement and with
     equal probability for every element. This avoids the two classic defects of naive lottery code: the modulo bias
     of rand() % N, which favours small numbers whenever N does not divide the generator range evenly, and the
     rejection loop over a std::set, whose running time is unbounded in the worst case. The sampled numbers are
     returned sorted, matching how lottery results are conventionally presented.

     The analysis routine draws the requested number of Monte Carlo tickets, accumulates per-number counts, and
     computes the chi-square statistic against the uniform expectation draws * kPicks / kPoolSize. The verdict flag
     compares the statistic with the 95 percent quantile for kPoolSize - 1 degrees of freedom, giving an automated
     pass judgement that the unit tests and the demonstration program both rely on.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned as value types.

 Supporting files:
     header      include/lottery.h
=========================================================================================================================
*/
#include "lottery.h"

#include <algorithm>
#include <numeric>

namespace lottery {

Draw make_draw(std::mt19937& rng) {
    // Build the pool 1..kPoolSize. iota fills sequentially starting at 1.
    std::array<int, kPoolSize> pool{};
    std::iota(pool.begin(), pool.end(), 1);

    Draw draw{};
    // std::sample picks kPicks elements without replacement, each subset
    // equally likely -- the mathematically correct lottery model. It also
    // preserves pool order, so the result is already ascending.
    std::sample(pool.begin(), pool.end(), draw.begin(), kPicks, rng);
    return draw;
}

FrequencyStats analyze_uniformity(std::mt19937& rng, std::int64_t draws) {
    FrequencyStats stats;
    stats.counts.assign(kPoolSize, 0);
    stats.total_draws = draws;

    for (std::int64_t d = 0; d < draws; ++d) {
        for (int number : make_draw(rng)) {
            ++stats.counts[static_cast<std::size_t>(number - 1)];
        }
    }

    // Chi-square against uniformity: expected count is identical for every
    // number because each subset of size kPicks is equally likely.
    const double expected =
        static_cast<double>(draws) * kPicks / kPoolSize;
    double chi = 0.0;
    for (std::int64_t observed : stats.counts) {
        const double diff = static_cast<double>(observed) - expected;
        chi += diff * diff / expected;
    }
    stats.chi_square = chi;
    stats.uniform_at_95 = (chi < kChiSquare95);
    return stats;
}

}  // namespace lottery
