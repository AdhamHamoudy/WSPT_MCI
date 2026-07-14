#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
using Clock = chrono::steady_clock;
using Cost = __int128_t;

struct Job {
    int id{};
    long long p{};  // Processing time on every machine
    long long w{};  // Weight / priority
};

struct ScheduleResult {
    Cost cost{};
    vector<Job> sequence;
};

struct TimingStats {
    double averageUs{};
    double medianUs{};
    double minimumUs{};
    double maximumUs{};
    double standardDeviationUs{};
};

string costToString(Cost value) {
    if (value == 0) {
        return "0";
    }

    const bool negative = value < 0;
    if (negative) {
        value = -value;
    }

    string result;
    while (value > 0) {
        const int digit = static_cast<int>(value % 10);
        result.push_back(static_cast<char>('0' + digit));
        value /= 10;
    }

    if (negative) {
        result.push_back('-');
    }

    reverse(result.begin(), result.end());
    return result;
}

Cost calculateTotalCost(const vector<Job>& sequence, int m) {
    Cost totalCost = 0;
    Cost prefixSumP = 0;
    long long prefixMaxP = 0;

    for (const Job& job : sequence) {
        prefixSumP += job.p;
        prefixMaxP = max(prefixMaxP, job.p);

        const Cost completionTime =
            prefixSumP + static_cast<Cost>(m - 1) * prefixMaxP;

        totalCost += static_cast<Cost>(job.w) * completionTime;
    }

    return totalCost;
}

// Exact WSPT comparison without floating-point division.
bool compareWSPT(const Job& a, const Job& b) {
    const Cost left = static_cast<Cost>(a.w) * b.p;
    const Cost right = static_cast<Cost>(b.w) * a.p;

    if (left != right) {
        return left > right;
    }

    // This matches the corrected WSPT-MCI.cpp tie rule.
    return a.id > b.id;
}

vector<size_t> findValidInsertionPositions(const vector<Job>& sequence,
                                           const Job& currentJob) {
    vector<size_t> validPositions;
    validPositions.reserve(sequence.size() + 1);

    // The end of the sequence is always a candidate.
    validPositions.push_back(sequence.size());

    long long runningMaxP = -1;
    for (size_t i = 0; i < sequence.size(); ++i) {
        if (sequence[i].p > runningMaxP) {
            runningMaxP = sequence[i].p;

            if (sequence[i].p >= currentJob.p) {
                validPositions.push_back(i);
            }
        }
    }

    return validPositions;
}

// Candidate under test: the corrected O(n^2) implementation from WSPT-MCI.cpp.
ScheduleResult runOptimizedWSPTMCI(vector<Job> jobs, int m) {
    stable_sort(jobs.begin(), jobs.end(), compareWSPT);

    if (jobs.empty()) {
        return {};
    }

    vector<Job> sequence;
    sequence.reserve(jobs.size());
    sequence.push_back(jobs.front());

    Cost currentTotalCost = calculateTotalCost(sequence, m);

    for (size_t jobIndex = 1; jobIndex < jobs.size(); ++jobIndex) {
        const Job& currentJob = jobs[jobIndex];
        const size_t k = sequence.size();

        // prefixSumP[r] and prefixMaxP[r] describe positions [0, r).
        vector<Cost> prefixSumP(k + 1, 0);
        vector<long long> prefixMaxP(k + 1, 0);

        for (size_t i = 0; i < k; ++i) {
            prefixSumP[i + 1] = prefixSumP[i] + sequence[i].p;
            prefixMaxP[i + 1] = max(prefixMaxP[i], sequence[i].p);
        }

        // suffixWeight[r] = sum of weights at positions [r, k).
        // suffixMaxIncrease[r] = weighted increase in prefix maxima caused by
        // inserting currentJob before positions [r, k).
        vector<Cost> suffixWeight(k + 1, 0);
        vector<Cost> suffixMaxIncrease(k + 1, 0);

        for (size_t i = k; i-- > 0;) {
            suffixWeight[i] = suffixWeight[i + 1] + sequence[i].w;

            const Cost maxIncrease = max<Cost>(
                0,
                static_cast<Cost>(currentJob.p) - prefixMaxP[i + 1]);

            suffixMaxIncrease[i] =
                suffixMaxIncrease[i + 1] +
                static_cast<Cost>(sequence[i].w) * maxIncrease;
        }

        const vector<size_t> validPositions =
            findValidInsertionPositions(sequence, currentJob);

        bool foundCandidate = false;
        Cost bestCost = 0;
        size_t bestPosition = 0;

        for (const size_t position : validPositions) {
            const Cost insertedCompletionTime =
                prefixSumP[position] + currentJob.p +
                static_cast<Cost>(m - 1) *
                    max(prefixMaxP[position], currentJob.p);

            const Cost candidateCost =
                currentTotalCost +
                static_cast<Cost>(currentJob.w) * insertedCompletionTime +
                static_cast<Cost>(currentJob.p) * suffixWeight[position] +
                static_cast<Cost>(m - 1) * suffixMaxIncrease[position];

            if (!foundCandidate || candidateCost < bestCost ||
                (candidateCost == bestCost && position > bestPosition)) {
                foundCandidate = true;
                bestCost = candidateCost;
                bestPosition = position;
            }
        }

        if (!foundCandidate) {
            throw logic_error("No valid insertion position was found.");
        }

        sequence.insert(sequence.begin() + static_cast<ptrdiff_t>(bestPosition),
                        currentJob);
        currentTotalCost = bestCost;
    }

    const Cost verifiedCost = calculateTotalCost(sequence, m);
    if (verifiedCost != currentTotalCost) {
        throw logic_error("Optimized incremental cost does not match a full recomputation.");
    }

    return {currentTotalCost, move(sequence)};
}

// Diagnostic reference: WSPT followed by MCI over every insertion position.
// This is not the global exact reference; it helps identify whether a mismatch
// comes from lemma filtering or from the more general insertion construction.
ScheduleResult runAllPositionMCI(vector<Job> jobs, int m) {
    stable_sort(jobs.begin(), jobs.end(), compareWSPT);

    if (jobs.empty()) {
        return {};
    }

    vector<Job> sequence;
    sequence.push_back(jobs.front());

    for (size_t jobIndex = 1; jobIndex < jobs.size(); ++jobIndex) {
        const Job& currentJob = jobs[jobIndex];

        bool foundCandidate = false;
        Cost bestCost = 0;
        size_t bestPosition = 0;
        vector<Job> bestSequence;

        for (size_t position = 0; position <= sequence.size(); ++position) {
            vector<Job> candidate = sequence;
            candidate.insert(candidate.begin() + static_cast<ptrdiff_t>(position),
                             currentJob);

            const Cost candidateCost = calculateTotalCost(candidate, m);

            if (!foundCandidate || candidateCost < bestCost ||
                (candidateCost == bestCost && position > bestPosition)) {
                foundCandidate = true;
                bestCost = candidateCost;
                bestPosition = position;
                bestSequence = move(candidate);
            }
        }

        sequence = move(bestSequence);
    }

    return {calculateTotalCost(sequence, m), move(sequence)};
}

// Independent exact reference: enumerate every permutation of every job.
ScheduleResult runTrueBruteForce(vector<Job> jobs, int m) {
    sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.id < b.id;
    });

    bool found = false;
    Cost bestCost = 0;
    vector<Job> bestSequence;

    do {
        const Cost candidateCost = calculateTotalCost(jobs, m);

        // For deterministic output, keep the first minimum permutation in ID order.
        if (!found || candidateCost < bestCost) {
            found = true;
            bestCost = candidateCost;
            bestSequence = jobs;
        }
    } while (next_permutation(jobs.begin(), jobs.end(),
                              [](const Job& a, const Job& b) {
                                  return a.id < b.id;
                              }));

    return {bestCost, move(bestSequence)};
}

void printSequence(const vector<Job>& sequence) {
    if (sequence.empty()) {
        cout << "None";
        return;
    }

    for (size_t i = 0; i < sequence.size(); ++i) {
        if (i != 0) {
            cout << " -> ";
        }
        cout << 'J' << sequence[i].id;
    }
}

void printDataset(const vector<Job>& jobs, int m) {
    cout << m << '\n';
    for (const Job& job : jobs) {
        cout << job.id << ' ' << job.p << ' ' << job.w << '\n';
    }
}

TimingStats calculateStats(const vector<double>& values) {
    TimingStats stats;
    if (values.empty()) {
        return stats;
    }

    stats.averageUs =
        accumulate(values.begin(), values.end(), 0.0) / values.size();

    const auto [minimumIt, maximumIt] =
        minmax_element(values.begin(), values.end());
    stats.minimumUs = *minimumIt;
    stats.maximumUs = *maximumIt;

    vector<double> sorted = values;
    sort(sorted.begin(), sorted.end());
    const size_t middle = sorted.size() / 2;
    if (sorted.size() % 2 == 0) {
        stats.medianUs = (sorted[middle - 1] + sorted[middle]) / 2.0;
    } else {
        stats.medianUs = sorted[middle];
    }

    double squaredDifferenceSum = 0.0;
    for (const double value : values) {
        const double difference = value - stats.averageUs;
        squaredDifferenceSum += difference * difference;
    }

    stats.standardDeviationUs =
        sqrt(squaredDifferenceSum / static_cast<double>(values.size()));

    return stats;
}

void printTimingRow(const string& name, const TimingStats& stats) {
    cout << left << setw(29) << name
         << right << setw(14) << fixed << setprecision(3) << stats.averageUs
         << setw(14) << stats.medianUs
         << setw(14) << stats.minimumUs
         << setw(14) << stats.maximumUs
         << setw(14) << stats.standardDeviationUs << '\n';
}

bool checkOneDataset(const vector<Job>& jobs,
                     int m,
                     int iteration,
                     uint64_t seed,
                     double& bruteUs,
                     double& allPositionMCIUs,
                     double& optimizedUs) {
    ScheduleResult bruteResult;
    ScheduleResult allPositionResult;
    ScheduleResult optimizedResult;

    // Rotate execution order to reduce systematic cache/order bias.
    const int order = iteration % 3;

    auto timeBrute = [&]() {
        const auto start = Clock::now();
        bruteResult = runTrueBruteForce(jobs, m);
        const auto stop = Clock::now();
        bruteUs = chrono::duration<double, micro>(stop - start).count();
    };

    auto timeAllPosition = [&]() {
        const auto start = Clock::now();
        allPositionResult = runAllPositionMCI(jobs, m);
        const auto stop = Clock::now();
        allPositionMCIUs =
            chrono::duration<double, micro>(stop - start).count();
    };

    auto timeOptimized = [&]() {
        const auto start = Clock::now();
        optimizedResult = runOptimizedWSPTMCI(jobs, m);
        const auto stop = Clock::now();
        optimizedUs = chrono::duration<double, micro>(stop - start).count();
    };

    if (order == 0) {
        timeBrute();
        timeAllPosition();
        timeOptimized();
    } else if (order == 1) {
        timeAllPosition();
        timeOptimized();
        timeBrute();
    } else {
        timeOptimized();
        timeBrute();
        timeAllPosition();
    }

    const bool optimizedMatchesBrute =
        optimizedResult.cost == bruteResult.cost;
    const bool allPositionMatchesBrute =
        allPositionResult.cost == bruteResult.cost;
    const bool optimizedMatchesAllPosition =
        optimizedResult.cost == allPositionResult.cost;

    if (optimizedMatchesBrute && allPositionMatchesBrute &&
        optimizedMatchesAllPosition) {
        return true;
    }

    cout << "\n[!] MISMATCH FOUND [!]\n";
    cout << "Seed: " << seed << '\n';
    cout << "Iteration: " << iteration << '\n';
    cout << "Machines: " << m << '\n';
    cout << "Jobs: " << jobs.size() << "\n\n";

    cout << "Reproducible dataset (save as jobs.txt):\n";
    printDataset(jobs, m);

    cout << "\nTrue brute-force cost: "
         << costToString(bruteResult.cost) << '\n';
    cout << "True brute-force sequence: ";
    printSequence(bruteResult.sequence);
    cout << '\n';

    cout << "\nAll-position MCI cost: "
         << costToString(allPositionResult.cost) << '\n';
    cout << "All-position MCI sequence: ";
    printSequence(allPositionResult.sequence);
    cout << '\n';

    cout << "\nOptimized WSPT-MCI cost: "
         << costToString(optimizedResult.cost) << '\n';
    cout << "Optimized WSPT-MCI sequence: ";
    printSequence(optimizedResult.sequence);
    cout << "\n\n";

    if (!allPositionMatchesBrute) {
        cout << "Diagnosis: even all-position MCI differs from global brute force.\n"
             << "The issue is not only lemma filtering; the insertion construction\n"
             << "or its theoretical assumptions need to be checked.\n";
    } else if (!optimizedMatchesAllPosition) {
        cout << "Diagnosis: all-position MCI matches brute force, but the optimized\n"
             << "version differs. Check the dominance-lemma filtering or the\n"
             << "incremental insertion-cost formula.\n";
    }

    return false;
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

    int maxJobs = 8;
    if (argc >= 4) {
        maxJobs = stoi(argv[3]);
    }

    if (maxJobs < 2 || maxJobs > 11) {
        cerr << "Error: maxJobs must be between 2 and 11.\n";
        return 1;
    }

    mt19937_64 generator(seed);

    uniform_int_distribution<int> jobCountDistribution(2, maxJobs);
    uniform_int_distribution<int> machineDistribution(1, 6);
    uniform_int_distribution<int> processingDistribution(1, 30);
    uniform_int_distribution<int> weightDistribution(0, 30);

    vector<double> bruteTimesUs;
    vector<double> allPositionTimesUs;
    vector<double> optimizedTimesUs;
    bruteTimesUs.reserve(iterations);
    allPositionTimesUs.reserve(iterations);
    optimizedTimesUs.reserve(iterations);

    cout << "Starting Part I WSPT-MCI differential fuzzer...\n";
    cout << "Reference: every permutation (true brute force)\n";
    cout << "Diagnostic: WSPT + all-position MCI\n";
    cout << "Candidate: optimized WSPT-MCI with lemma filtering\n";
    cout << "Iterations: " << iterations << '\n';
    cout << "Maximum jobs per dataset: " << maxJobs << '\n';
    cout << "Seed: " << seed << '\n';
    cout << "----------------------------------------\n";

    for (int iteration = 1; iteration <= iterations; ++iteration) {
        const int n = jobCountDistribution(generator);
        const int m = machineDistribution(generator);

        vector<Job> jobs;
        jobs.reserve(static_cast<size_t>(n));

        for (int i = 0; i < n; ++i) {
            jobs.push_back({
                i + 1,
                processingDistribution(generator),
                weightDistribution(generator)
            });
        }

        double bruteUs = 0.0;
        double allPositionUs = 0.0;
        double optimizedUs = 0.0;

        try {
            if (!checkOneDataset(jobs,
                                 m,
                                 iteration,
                                 seed,
                                 bruteUs,
                                 allPositionUs,
                                 optimizedUs)) {
                cout << "\nRerun the same random stream with:\n";
                cout << "./fuzzer_part1 " << iterations << ' ' << seed << ' '
                     << maxJobs << '\n';
                return 1;
            }
        } catch (const exception& error) {
            cerr << "\nInternal error at iteration " << iteration
                 << ": " << error.what() << '\n';
            cout << "Dataset:\n";
            printDataset(jobs, m);
            return 1;
        }

        bruteTimesUs.push_back(bruteUs);
        allPositionTimesUs.push_back(allPositionUs);
        optimizedTimesUs.push_back(optimizedUs);

        if (iteration % 100 == 0 || iteration == iterations) {
            cout << "Passed " << iteration << " datasets.\n";
        }
    }

    const TimingStats bruteStats = calculateStats(bruteTimesUs);
    const TimingStats allPositionStats = calculateStats(allPositionTimesUs);
    const TimingStats optimizedStats = calculateStats(optimizedTimesUs);

    cout << "----------------------------------------\n";
    cout << "SUCCESS: Every randomized dataset matched exactly.\n\n";

    cout << "Timing statistics per dataset (microseconds)\n";
    cout << left << setw(29) << "Algorithm"
         << right << setw(14) << "Average"
         << setw(14) << "Median"
         << setw(14) << "Minimum"
         << setw(14) << "Maximum"
         << setw(14) << "Std. dev." << '\n';
    cout << string(99, '-') << '\n';
    printTimingRow("True brute force", bruteStats);
    printTimingRow("All-position MCI", allPositionStats);
    printTimingRow("Optimized WSPT-MCI", optimizedStats);

    if (optimizedStats.averageUs > 0.0) {
        cout << "\nAverage brute-force/optimized speedup: "
             << fixed << setprecision(2)
             << bruteStats.averageUs / optimizedStats.averageUs << "x\n";
    }

    cout << "\nFor reproducibility, rerun with:\n";
    cout << "./fuzzer_part1 " << iterations << ' ' << seed << ' '
         << maxJobs << '\n';

    return 0;
}