/*
=========================================================================================================================
 *** lottery.h ***
 Core interfaces for unbiased lottery draw generation and Monte Carlo uniformity analysis.
=========================================================================================================================

 Description:
     This header declares the two building blocks of the project. The first block is the draw generator, which
     produces one lottery draw of kPicks distinct numbers from 1 to kPoolSize using std::sample over the candidate
     pool, so every number has an equal selection probability by construction. The second block is the analysis
     layer, which runs a configurable number of Monte Carlo draws, accumulates a frequency count per number, and
     evaluates the empirical distribution with a chi-square goodness-of-fit statistic against the uniform hypothesis.

     The chi-square statistic is computed as the sum over all numbers of (observed - expected)^2 / expected, where
     the expected count per number is draws * kPicks / kPoolSize under uniformity. For kPoolSize - 1 = 48 degrees of
     freedom, the statistic should fall below 65.17 in 95 percent of runs when the generator is truly uniform, so a
     single threshold comparison gives a meaningful automated verdict.

 Input:
     (none)         Interfaces only; parameters are passed by the callers.

 Output:
     (none)         Interfaces only; results are returned as value types.

 Supporting files:
     source      src/lottery.cpp (implementations)
     tests       tests/test_lottery.cpp (unit tests for draw validity and statistics)
=========================================================================================================================
*/
#ifndef LOTTERY_H
#define LOTTERY_H

#include <array>
#include <cstdint>
#include <random>
#include <vector>

namespace lottery {

constexpr int kPoolSize = 49;   // Numbers 1..49.
constexpr int kPicks    = 7;    // Distinct numbers per draw.

// 95th percentile of the chi-square distribution with 48 degrees of freedom
// (kPoolSize - 1). A uniform generator stays below this in 95% of runs.
constexpr double kChiSquare95 = 65.17;

using Draw = std::array<int, kPicks>;

// One draw of kPicks distinct numbers in ascending order.
// The engine is passed by reference so tests can use a fixed seed.
Draw make_draw(std::mt19937& rng);

struct FrequencyStats {
    std::vector<std::int64_t> counts;   // counts[i] = occurrences of number i+1.
    std::int64_t total_draws = 0;
    double chi_square = 0.0;
    bool uniform_at_95 = false;         // chi_square below the 95% threshold.
};

// Run 'draws' Monte Carlo draws and evaluate uniformity.
FrequencyStats analyze_uniformity(std::mt19937& rng, std::int64_t draws);

}  // namespace lottery

#endif  // LOTTERY_H
