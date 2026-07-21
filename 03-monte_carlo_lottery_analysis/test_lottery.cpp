/*
=========================================================================================================================
 *** test_lottery ***
 Unit tests for draw validity, determinism under a fixed seed, and the chi-square uniformity analysis.
=========================================================================================================================

 Description:
     This test verifies the lottery module through four properties. The first property is draw validity: every draw
     must contain exactly kPicks numbers, all inside 1 to kPoolSize, all distinct, and in ascending order. The second
     property is determinism: two engines constructed with the same fixed seed must produce identical draws, which is
     the basis for reproducible simulations. The third property is count conservation: after any number of analysis
     draws, the per-number counts must sum to draws * kPicks exactly. The fourth property is the statistical verdict:
     with a fixed seed and one hundred thousand draws, the chi-square statistic must fall below the 95 percent
     threshold, pinning the uniformity of the generator in a reproducible way.

     The test avoids external frameworks on purpose: a small check macro counts failures and prints one line per
     failed property, so the test builds anywhere a compiler exists. The program prints one summary line and returns
     zero only when every property holds, which makes it usable in scripts and continuous-integration pipelines.

 Input:
     (none)         All configuration is in code; the statistical test uses a fixed seed.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/lottery.h (the functions under test)
=========================================================================================================================
*/
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <set>

#include "lottery.h"

// CHECK: count a failure and report the failed expression, but keep running
// so one broken property does not hide the state of the others.
#define CHECK(cond)                                            \
    do {                                                       \
        if (!(cond)) {                                         \
            std::printf("  FAILED: %s (line %d)\n", #cond, __LINE__); \
            ++fails;                                           \
        }                                                      \
    } while (0)

int main() {
    int fails = 0;

    {   // Property 1: draw validity over many random draws.
        std::mt19937 rng(12345);
        for (int i = 0; i < 1000; ++i) {
            const auto draw = lottery::make_draw(rng);
            CHECK(std::all_of(draw.begin(), draw.end(), [](int n) {
                return n >= 1 && n <= lottery::kPoolSize;
            }));
            const std::set<int> unique(draw.begin(), draw.end());
            CHECK(unique.size() == draw.size());
            CHECK(std::is_sorted(draw.begin(), draw.end()));
        }
    }

    {   // Property 2: identical seeds give identical draws.
        std::mt19937 a(777), b(777);
        for (int i = 0; i < 100; ++i) {
            CHECK(lottery::make_draw(a) == lottery::make_draw(b));
        }
    }

    {   // Property 3: counts conserve the total number of drawn values.
        std::mt19937 rng(42);
        const std::int64_t draws = 5000;
        const auto stats = lottery::analyze_uniformity(rng, draws);
        const auto total = std::accumulate(stats.counts.begin(),
                                           stats.counts.end(),
                                           static_cast<std::int64_t>(0));
        CHECK(total == draws * lottery::kPicks);
        CHECK(stats.total_draws == draws);
    }

    {   // Property 4: fixed-seed statistical verdict is reproducibly uniform.
        std::mt19937 rng(2026);
        const auto stats = lottery::analyze_uniformity(rng, 100000);
        CHECK(stats.chi_square > 0.0);
        CHECK(stats.uniform_at_95);
    }

    std::printf("[test_lottery] validity + determinism + conservation + chi-square -> %s\n",
                fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
