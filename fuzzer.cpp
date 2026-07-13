#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

using namespace std;
using Clock = chrono::steady_clock;

struct Job {
    int id{};
    long long p{};
    long long w{};
    long long e{};
};

constexpr long long INF = numeric_limits<long long>::max() / 4;

long long calculateTotalCost(const vector<Job>& sequence, int m) {
    long long totalCost = 0;
    long long sumP = 0;
    long long maxP = 0;

    for (const Job& job : sequence) {
        sumP += job.p;
        maxP = max(maxP, job.p);
        const long long completion =
            sumP + static_cast<long long>(m - 1) * maxP;

        const __int128 candidate = static_cast<__int128>(totalCost) +
                                   static_cast<__int128>(job.w) * completion;
        if (candidate >= INF) {
            return INF;
        }
        totalCost = static_cast<long long>(candidate);
    }

    return totalCost;
}

// Independent exact reference for small n:
// enumerate every accepted subset and every permutation of that subset.
long long runTrueBruteForce(const vector<Job>& jobs, int m, long long U) {
    const int n = static_cast<int>(jobs.size());
    if (n >= 63) {
        return INF;
    }

    const uint64_t totalMasks = uint64_t{1} << n;
    long long totalRejectionCost = 0;
    for (const Job& job : jobs) {
        totalRejectionCost += job.e;
    }

    long long best = INF;

    for (uint64_t mask = 0; mask < totalMasks; ++mask) {
        long long acceptedRejectionCost = 0;
        vector<int> acceptedIndices;

        for (int i = 0; i < n; ++i) {
            if ((mask & (uint64_t{1} << i)) != 0) {
                acceptedIndices.push_back(i);
                acceptedRejectionCost += jobs[i].e;
            }
        }

        const long long rejectionSpent =
            totalRejectionCost - acceptedRejectionCost;
        if (rejectionSpent > U) {
            continue;
        }

        if (acceptedIndices.empty()) {
            best = 0;
            continue;
        }

        sort(acceptedIndices.begin(), acceptedIndices.end());
        do {
            vector<Job> sequence;
            sequence.reserve(acceptedIndices.size());
            for (int index : acceptedIndices) {
                sequence.push_back(jobs[index]);
            }
            best = min(best, calculateTotalCost(sequence, m));
        } while (next_permutation(acceptedIndices.begin(), acceptedIndices.end()));
    }

    return best;
}

// Exact subset DP:
// dp[S] = minimum scheduling cost of arranging exactly the jobs in subset S.
long long runExactSubsetDP(const vector<Job>& jobs, int m, long long U) {
    const int n = static_cast<int>(jobs.size());
    if (n >= 63) {
        return INF;
    }

    const uint64_t totalStates = uint64_t{1} << n;

    long long totalRejectionCost = 0;
    for (const Job& job : jobs) {
        totalRejectionCost += job.e;
    }

    vector<long long> sumP(totalStates, 0);
    vector<long long> maxP(totalStates, 0);
    vector<long long> acceptedE(totalStates, 0);
    vector<long long> dp(totalStates, INF);
    dp[0] = 0;

    long long answer = (totalRejectionCost <= U) ? 0 : INF;

    for (uint64_t mask = 1; mask < totalStates; ++mask) {
        const uint64_t leastBit = mask & (~mask + 1);
        const int j = __builtin_ctzll(leastBit);
        const uint64_t previousMask = mask ^ leastBit;

        sumP[mask] = sumP[previousMask] + jobs[j].p;
        maxP[mask] = max(maxP[previousMask], jobs[j].p);
        acceptedE[mask] = acceptedE[previousMask] + jobs[j].e;

        const long long completion =
            sumP[mask] + static_cast<long long>(m - 1) * maxP[mask];

        uint64_t bits = mask;
        while (bits != 0) {
            const int last = __builtin_ctzll(bits);
            const uint64_t bit = uint64_t{1} << last;
            const uint64_t prefix = mask ^ bit;

            if (dp[prefix] != INF) {
                const __int128 candidate =
                    static_cast<__int128>(dp[prefix]) +
                    static_cast<__int128>(jobs[last].w) * completion;

                if (candidate < dp[mask]) {
                    dp[mask] = static_cast<long long>(candidate);
                }
            }

            bits ^= bit;
        }

        const long long rejectionSpent =
            totalRejectionCost - acceptedE[mask];
        if (rejectionSpent <= U) {
            answer = min(answer, dp[mask]);
        }
    }

    return answer;
}

void printDataset(const vector<Job>& jobs, int m, long long U) {
    cout << m << ' ' << U << '\n';
    for (const Job& job : jobs) {
        cout << job.id << ' ' << job.p << ' ' << job.w << ' ' << job.e << '\n';
    }
}

struct TimingStats {
    double averageUs{};
    double medianUs{};
    double minUs{};
    double maxUs{};
    double standardDeviationUs{};
};

TimingStats calculateStats(const vector<double>& values) {
    TimingStats stats;
    if (values.empty()) {
        return stats;
    }

    stats.averageUs =
        accumulate(values.begin(), values.end(), 0.0) / values.size();

    const auto [minIt, maxIt] = minmax_element(values.begin(), values.end());
    stats.minUs = *minIt;
    stats.maxUs = *maxIt;

    vector<double> sorted = values;
    sort(sorted.begin(), sorted.end());
    const size_t middle = sorted.size() / 2;
    if (sorted.size() % 2 == 0) {
        stats.medianUs = (sorted[middle - 1] + sorted[middle]) / 2.0;
    } else {
        stats.medianUs = sorted[middle];
    }

    double squaredDifferenceSum = 0.0;
    for (double value : values) {
        const double difference = value - stats.averageUs;
        squaredDifferenceSum += difference * difference;
    }
    stats.standardDeviationUs =
        sqrt(squaredDifferenceSum / static_cast<double>(values.size()));

    return stats;
}

void printTimingRow(const string& name, const TimingStats& stats) {
    cout << left << setw(28) << name
         << right << setw(14) << fixed << setprecision(3) << stats.averageUs
         << setw(14) << stats.medianUs
         << setw(14) << stats.minUs
         << setw(14) << stats.maxUs
         << setw(14) << stats.standardDeviationUs << '\n';
}

int main(int argc, char* argv[]) {
    int iterations = 1000;
    if (argc >= 2) {
        iterations = max(1, stoi(argv[1]));
    }

    uint64_t seed = static_cast<uint64_t>(
        chrono::high_resolution_clock::now().time_since_epoch().count());
    if (argc >= 3) {
        seed = stoull(argv[2]);
    }

    mt19937_64 generator(seed);

    uniform_int_distribution<int> pDistribution(1, 30);
    uniform_int_distribution<int> wDistribution(1, 30);
    uniform_int_distribution<int> eDistribution(1, 15);
    uniform_int_distribution<int> machineDistribution(1, 6);
    uniform_int_distribution<int> budgetDistribution(0, 45);

    // Keep n small because the independent reference enumerates permutations.
    constexpr int n = 8;

    vector<double> bruteForceTimesUs;
    vector<double> subsetDPTimesUs;
    bruteForceTimesUs.reserve(iterations);
    subsetDPTimesUs.reserve(iterations);

    cout << "Starting corrected fuzzer with timing...\n";
    cout << "Reference: all subsets and all permutations\n";
    cout << "Candidate: exact subset DP O(n * 2^n)\n";
    cout << "Jobs per dataset: " << n << "\n";
    cout << "Seed: " << seed << "\n";
    cout << "Iterations: " << iterations << "\n";
    cout << "----------------------------------------\n";

    for (int iteration = 1; iteration <= iterations; ++iteration) {
        const int m = machineDistribution(generator);
        const long long U = budgetDistribution(generator);

        vector<Job> jobs;
        jobs.reserve(n);
        for (int i = 0; i < n; ++i) {
            jobs.push_back({
                i + 1,
                pDistribution(generator),
                wDistribution(generator),
                eDistribution(generator)
            });
        }

        long long bruteCost = INF;
        long long dpCost = INF;
        double bruteUs = 0.0;
        double dpUs = 0.0;

        // Alternate execution order to reduce systematic cache/order bias.
        if (iteration % 2 == 1) {
            const auto bruteStart = Clock::now();
            bruteCost = runTrueBruteForce(jobs, m, U);
            const auto bruteStop = Clock::now();
            bruteUs = chrono::duration<double, micro>(bruteStop - bruteStart).count();

            const auto dpStart = Clock::now();
            dpCost = runExactSubsetDP(jobs, m, U);
            const auto dpStop = Clock::now();
            dpUs = chrono::duration<double, micro>(dpStop - dpStart).count();
        } else {
            const auto dpStart = Clock::now();
            dpCost = runExactSubsetDP(jobs, m, U);
            const auto dpStop = Clock::now();
            dpUs = chrono::duration<double, micro>(dpStop - dpStart).count();

            const auto bruteStart = Clock::now();
            bruteCost = runTrueBruteForce(jobs, m, U);
            const auto bruteStop = Clock::now();
            bruteUs = chrono::duration<double, micro>(bruteStop - bruteStart).count();
        }

        bruteForceTimesUs.push_back(bruteUs);
        subsetDPTimesUs.push_back(dpUs);

        if (bruteCost != dpCost) {
            cout << "\n[!] MISMATCH FOUND [!]\n";
            cout << "Iteration: " << iteration << '\n';
            cout << "True brute-force cost: " << bruteCost << '\n';
            cout << "Exact subset-DP cost: " << dpCost << "\n\n";
            cout << "Dataset:\n";
            printDataset(jobs, m, U);
            return 1;
        }

        if (iteration % 100 == 0) {
            cout << "Passed " << iteration << " datasets.\n";
        }
    }

    const TimingStats bruteStats = calculateStats(bruteForceTimesUs);
    const TimingStats dpStats = calculateStats(subsetDPTimesUs);

    cout << "----------------------------------------\n";
    cout << "SUCCESS: Every dataset matched exactly.\n\n";

    cout << "Timing statistics per dataset (microseconds)\n";
    cout << left << setw(28) << "Algorithm"
         << right << setw(14) << "Average"
         << setw(14) << "Median"
         << setw(14) << "Minimum"
         << setw(14) << "Maximum"
         << setw(14) << "Std. dev." << '\n';
    cout << string(98, '-') << '\n';
    printTimingRow("True brute force", bruteStats);
    printTimingRow("Exact subset DP", dpStats);

    if (dpStats.averageUs > 0.0) {
        cout << "\nAverage speedup: " << fixed << setprecision(2)
             << bruteStats.averageUs / dpStats.averageUs << "x\n";
    }

    cout << "\nFor reproducibility, rerun with:\n";
    cout << "./fuzzer " << iterations << ' ' << seed << '\n';

    return 0;
}
