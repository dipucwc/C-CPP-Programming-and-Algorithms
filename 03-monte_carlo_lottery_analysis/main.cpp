/*
=========================================================================================================================
 *** main.cpp ***
 Demonstration program: one lottery draw, then a Monte Carlo uniformity analysis with a chi-square verdict.
=========================================================================================================================

 Description:
     This program demonstrates the lottery module in two stages. In the first stage, it prints one draw of seven
     distinct numbers between 1 and 49, which is the original task of the lottery generator. In the second stage, it
     runs one hundred thousand Monte Carlo draws, prints the frequency extremes across the 49 numbers, and reports
     the chi-square statistic together with the 95 percent uniformity verdict, turning the toy generator into a
     statistically verified one.

     The engine is seeded from the hardware random device for the demonstration, so every run produces a different
     draw and a slightly different statistic. The verdict is expected to report uniform in roughly 19 of 20 runs by
     the definition of the 95 percent threshold; an occasional exceedance is normal and not a defect.

 Input:
     (none)         The draw count is set in code.

 Output:
     return value   Zero on normal completion.
     stdout         The single draw, the frequency extremes, the chi-square value, and the uniformity verdict.

 Supporting files:
     header      include/lottery.h
=========================================================================================================================
*/
#include <algorithm>
#include <iostream>
#include <iomanip>

#include "lottery.h"

int main() {
    std::mt19937 rng(std::random_device{}());

    std::cout << "Your lottery numbers are:";
    for (int n : lottery::make_draw(rng)) std::cout << ' ' << n;
    std::cout << "\n\n";

    constexpr std::int64_t kDraws = 100000;
    const auto stats = lottery::analyze_uniformity(rng, kDraws);

    const auto [min_it, max_it] =
        std::minmax_element(stats.counts.begin(), stats.counts.end());
    const double expected =
        static_cast<double>(kDraws) * lottery::kPicks / lottery::kPoolSize;

    std::cout << "Monte Carlo uniformity check (" << kDraws << " draws)\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  expected count per number : " << expected << "\n";
    std::cout << "  observed min / max        : " << *min_it << " / " << *max_it << "\n";
    std::cout << "  chi-square (48 dof)       : " << stats.chi_square
              << "  (95% threshold " << lottery::kChiSquare95 << ")\n";
    std::cout << "  verdict                   : "
              << (stats.uniform_at_95 ? "UNIFORM at 95% confidence"
                                      : "exceeds 95% threshold (expected in ~5% of runs)")
              << "\n";

    return 0;
}
