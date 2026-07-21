/*
=========================================================================================================================
 *** random_distributions ***
 Sampler for uniform and normal distributions with statistical verification against theoretical moments.
=========================================================================================================================

 Description:
     This program draws a large number of samples from a uniform integer distribution and from a normal distribution,
     then verifies the generator quality by comparing the empirical mean and standard deviation of each sample set
     against the theoretical values of the configured distributions. For the uniform distribution over 1 to 20, the
     theoretical mean is 10.5 and the theoretical standard deviation follows from the discrete uniform variance
     formula. For the normal distribution, the configured mean 10.0 and standard deviation 1.2 are the references
     directly. Agreement within a small tolerance confirms that the engine and distributions behave as specified.

     The complete procedure runs in three parts. In the first part, a Mersenne Twister engine is seeded from the
     hardware random device and one hundred thousand samples are drawn from each distribution into separate vectors.
     In the second part, the empirical mean and standard deviation of each vector are computed in a single pass over
     the data. In the third part, a compact text histogram of the normal samples is printed, showing the expected
     bell shape centred on the configured mean, and the empirical statistics are reported next to their theoretical
     values so the agreement can be read directly from the output.

 Input:
     (none)         The sample count and distribution parameters are set in code.

 Output:
     return value   Zero on normal completion.
     stdout         Empirical versus theoretical statistics for both distributions and a text histogram.

 Supporting files:
     header      <random> for the engine and distributions, <cmath> for the square root
     reference   discrete uniform variance ((b - a + 1)^2 - 1) / 12; normal parameters are the references directly
=========================================================================================================================
*/
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <map>
#include <cmath>

namespace stats {

constexpr int    kSamples     = 100000;
constexpr int    kUniformLow  = 1;
constexpr int    kUniformHigh = 20;
constexpr double kNormalMean  = 10.0;
constexpr double kNormalStd   = 1.2;

struct Moments {
    double mean = 0.0;
    double std_dev = 0.0;
};

Moments compute_moments(const std::vector<double>& v) {
    double sum = 0.0, sum_sq = 0.0;
    for (double x : v) {
        sum += x;
        sum_sq += x * x;
    }
    const double n = static_cast<double>(v.size());
    const double mean = sum / n;
    // Variance via E[x^2] - (E[x])^2; adequate here since values are near the
    // mean and n is large, so catastrophic cancellation is not a concern.
    const double var = sum_sq / n - mean * mean;
    return {mean, std::sqrt(var)};
}

void print_histogram(const std::vector<double>& v) {
    std::map<int, int> bins;
    for (double x : v) ++bins[static_cast<int>(std::lround(x))];

    for (const auto& [value, count] : bins) {
        // Scale: one '*' per 200 samples keeps the widest bar on one line.
        std::cout << std::setw(4) << value << " | "
                  << std::string(count / 200, '*') << "\n";
    }
}

}  // namespace stats

int main() {
    std::mt19937 rng(std::random_device{}());

    std::uniform_int_distribution<int> uniform(stats::kUniformLow, stats::kUniformHigh);
    std::normal_distribution<double>   normal(stats::kNormalMean, stats::kNormalStd);

    std::vector<double> u_samples, n_samples;
    u_samples.reserve(stats::kSamples);
    n_samples.reserve(stats::kSamples);

    for (int i = 0; i < stats::kSamples; ++i) {
        u_samples.push_back(static_cast<double>(uniform(rng)));
        n_samples.push_back(normal(rng));
    }

    const auto u = stats::compute_moments(u_samples);
    const auto n = stats::compute_moments(n_samples);

    // Theoretical references: discrete uniform mean (a+b)/2 and
    // variance ((b - a + 1)^2 - 1) / 12.
    const double span = stats::kUniformHigh - stats::kUniformLow + 1.0;
    const double u_mean_theory = 0.5 * (stats::kUniformLow + stats::kUniformHigh);
    const double u_std_theory  = std::sqrt((span * span - 1.0) / 12.0);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Uniform[" << stats::kUniformLow << "," << stats::kUniformHigh << "]  "
              << "mean " << u.mean << " (theory " << u_mean_theory << "),  "
              << "std "  << u.std_dev << " (theory " << u_std_theory << ")\n";
    std::cout << "Normal(" << stats::kNormalMean << "," << stats::kNormalStd << ")  "
              << "mean " << n.mean << " (theory " << stats::kNormalMean << "),  "
              << "std "  << n.std_dev << " (theory " << stats::kNormalStd << ")\n\n";

    std::cout << "Histogram of normal samples:\n";
    stats::print_histogram(n_samples);

    return 0;
}