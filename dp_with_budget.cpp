#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace std;
using namespace std::chrono;

struct Job {
    int id{};
    long long p{};  // Processing time
    long long w{};  // Weight / priority
    long long e{};  // Rejection cost
};

/*
    Correct exact solver for:

        minimize  sum_j w_j C_j
        subject to total rejection cost <= U

    in a proportionate flow shop, where for a sequence prefix S:

        C = sum_{j in S} p_j + (m - 1) * max_{j in S} p_j.

    Why this differs from the original program:
    -------------------------------------------
    A single DP cell dp[i][b] is not enough. Two partial solutions with the
    same i and b can have different cumulative processing times, maximum
    processing times, and future costs. Keeping only the currently cheaper
    one can delete the globally optimal solution.

    This implementation uses an exact subset DP. For every accepted subset S,
    dp[S] stores the minimum schedule cost of arranging exactly the jobs in S.

    If job j is last in S, its completion time depends only on the set S:

        completion(S) = sumP(S) + (m - 1) * maxP(S).

    Therefore:

        dp[S] = min over j in S {
                    dp[S without j] + w_j * completion(S)
                }.

    The final answer is the feasible accepted subset S for which:

        totalRejectionCost - acceptedRejectionCost(S) <= U

    and dp[S] is minimum.

    Complexity:
        Time   O(n * 2^n)
        Memory O(2^n)

    This is exact. The original claimed O(n * U) recurrence is not exact for
    this objective because its state does not contain enough information.
*/

namespace {
constexpr int MAX_EXACT_JOBS = 24;
constexpr long long INF = numeric_limits<long long>::max() / 4;

bool addWillOverflow(long long a, long long b) {
    return b > 0 && a > numeric_limits<long long>::max() - b;
}

long long safeCandidateCost(long long prefixCost,
                            long long weight,
                            long long completionTime) {
    const __int128 candidate = static_cast<__int128>(prefixCost) +
                               static_cast<__int128>(weight) * completionTime;

    if (candidate >= INF) {
        return INF;
    }
    return static_cast<long long>(candidate);
}
}  // namespace

int main(int argc, char* argv[]) {
    const string inputFileName =
        (argc >= 2) ? argv[1] : "jobs_with_budget.txt";

    ifstream inputFile(inputFileName);
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open " << inputFileName << "!\n";
        return 1;
    }

    int m = 0;
    long long U = 0;
    if (!(inputFile >> m >> U)) {
        cerr << "Error: First line must contain: <machines> <budget>.\n";
        return 1;
    }

    if (m <= 0) {
        cerr << "Error: Number of machines must be positive.\n";
        return 1;
    }
    if (U < 0) {
        cerr << "Error: Rejection budget cannot be negative.\n";
        return 1;
    }

    vector<Job> jobs;
    Job job;
    long long totalRejectionCost = 0;

    while (inputFile >> job.id >> job.p >> job.w >> job.e) {
        if (job.p <= 0) {
            cerr << "Error: Job J" << job.id
                 << " has a non-positive processing time.\n";
            return 1;
        }
        if (job.w < 0 || job.e < 0) {
            cerr << "Error: Job J" << job.id
                 << " has a negative weight or rejection cost.\n";
            return 1;
        }
        if (addWillOverflow(totalRejectionCost, job.e)) {
            cerr << "Error: Total rejection cost exceeds 64-bit range.\n";
            return 1;
        }

        totalRejectionCost += job.e;
        jobs.push_back(job);
    }

    if (!inputFile.eof()) {
        cerr << "Error: Invalid job record. Expected: <id> <p> <w> <e>.\n";
        return 1;
    }

    const int n = static_cast<int>(jobs.size());
    if (n == 0) {
        cout << "No jobs were loaded. The optimal cost is 0.\n";
        return 0;
    }

    if (n > MAX_EXACT_JOBS) {
        cerr << "Error: This exact subset-DP implementation supports at most "
             << MAX_EXACT_JOBS << " jobs.\n"
             << "Loaded jobs: " << n << "\n"
             << "Reason: an exact solution requires O(2^n) states; the original "
                "O(n * U) state is not mathematically sufficient.\n";
        return 1;
    }

    const uint64_t totalStates = uint64_t{1} << n;

    cout << "Loaded " << n << " jobs for " << m << " machines.\n";
    cout << "Maximum Rejection Budget (U): " << U << "\n";
    cout << "Running exact subset DP with " << totalStates << " states...\n";
    cout << "----------------------------------------\n";

    const auto start = high_resolution_clock::now();

    vector<long long> dp(static_cast<size_t>(totalStates), INF);
    vector<int8_t> lastJob(static_cast<size_t>(totalStates), int8_t{-1});
    dp[0] = 0;

    long long globalMinCost = INF;
    long long bestBudgetSpent = 0;
    uint64_t bestAcceptedMask = 0;

    // Empty accepted subset: all jobs are rejected.
    if (totalRejectionCost <= U) {
        globalMinCost = 0;
        bestBudgetSpent = totalRejectionCost;
        bestAcceptedMask = 0;
    }

    for (uint64_t mask = 1; mask < totalStates; ++mask) {
        long long sumP = 0;
        long long maxP = 0;
        long long acceptedRejectionCost = 0;

        uint64_t bits = mask;
        while (bits != 0) {
            const int j = __builtin_ctzll(bits);
            const uint64_t bit = uint64_t{1} << j;

            if (addWillOverflow(sumP, jobs[j].p) ||
                addWillOverflow(acceptedRejectionCost, jobs[j].e)) {
                cerr << "Error: Aggregate input values exceed 64-bit range.\n";
                return 1;
            }

            sumP += jobs[j].p;
            maxP = max(maxP, jobs[j].p);
            acceptedRejectionCost += jobs[j].e;
            bits ^= bit;
        }

        const __int128 completion128 =
            static_cast<__int128>(sumP) +
            static_cast<__int128>(m - 1) * maxP;

        if (completion128 >= INF) {
            cerr << "Error: Completion time exceeds supported 64-bit range.\n";
            return 1;
        }
        const long long completionTime =
            static_cast<long long>(completion128);

        long long bestCostForMask = INF;
        int bestLastJob = -1;

        bits = mask;
        while (bits != 0) {
            const int j = __builtin_ctzll(bits);
            const uint64_t bit = uint64_t{1} << j;
            const uint64_t previousMask = mask ^ bit;

            const long long candidate = safeCandidateCost(
                dp[static_cast<size_t>(previousMask)],
                jobs[j].w,
                completionTime);

            if (candidate < bestCostForMask ||
                (candidate == bestCostForMask &&
                 (bestLastJob == -1 || jobs[j].id < jobs[bestLastJob].id))) {
                bestCostForMask = candidate;
                bestLastJob = j;
            }

            bits ^= bit;
        }

        dp[static_cast<size_t>(mask)] = bestCostForMask;
        lastJob[static_cast<size_t>(mask)] =
            static_cast<int8_t>(bestLastJob);

        const long long rejectionSpent =
            totalRejectionCost - acceptedRejectionCost;

        if (rejectionSpent <= U &&
            (bestCostForMask < globalMinCost ||
             (bestCostForMask == globalMinCost &&
              rejectionSpent < bestBudgetSpent))) {
            globalMinCost = bestCostForMask;
            bestBudgetSpent = rejectionSpent;
            bestAcceptedMask = mask;
        }
    }

    if (globalMinCost == INF) {
        cout << "No feasible solution exists for budget U = " << U << ".\n";
        return 0;
    }

    vector<Job> optimalSequenceReversed;
    uint64_t reconstructionMask = bestAcceptedMask;

    while (reconstructionMask != 0) {
        const int j = lastJob[static_cast<size_t>(reconstructionMask)];
        if (j < 0) {
            cerr << "Internal error while reconstructing the optimal schedule.\n";
            return 1;
        }

        optimalSequenceReversed.push_back(jobs[j]);
        reconstructionMask ^= uint64_t{1} << j;
    }

    reverse(optimalSequenceReversed.begin(), optimalSequenceReversed.end());

    vector<int> rejectedJobIds;
    for (int j = 0; j < n; ++j) {
        if ((bestAcceptedMask & (uint64_t{1} << j)) == 0) {
            rejectedJobIds.push_back(jobs[j].id);
        }
    }
    sort(rejectedJobIds.begin(), rejectedJobIds.end());

    const auto stop = high_resolution_clock::now();
    const auto durationUs = duration_cast<microseconds>(stop - start).count();
    const auto durationMs = duration_cast<milliseconds>(stop - start).count();

    cout << "[EXACT OPTIMAL SOLUTION FOUND]\n";
    cout << "Total Rejection Cost Spent: " << bestBudgetSpent
         << " / " << U << "\n";
    cout << "Total Minimized Schedule Cost: " << globalMinCost << "\n";

    cout << "Kept Jobs in Optimal Schedule: ";
    if (optimalSequenceReversed.empty()) {
        cout << "None";
    } else {
        for (const Job& keptJob : optimalSequenceReversed) {
            cout << "J" << keptJob.id << ' ';
        }
    }
    cout << '\n';

    cout << "Rejected Jobs: ";
    if (rejectedJobIds.empty()) {
        cout << "None";
    } else {
        for (int id : rejectedJobIds) {
            cout << "J" << id << ' ';
        }
    }
    cout << "\n\n";

    cout << "DP Execution Time: " << durationUs << " microseconds ("
         << durationMs << " ms)\n";
    cout << "========================================\n";

    return 0;
}
