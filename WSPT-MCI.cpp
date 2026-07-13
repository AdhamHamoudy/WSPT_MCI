// #include <iostream>
// #include <vector>
// #include <algorithm>
// #include <climits>
// #include <fstream>

// using namespace std;

// struct Job {
//     int id;
//     int p;
//     int w;
// };

// // Shakhlevich Formula Function
// int calculateTotalCost(const vector<Job>& sequence, int m) {
//     int totalCost = 0;
//     int sumP = 0;
//     int maxP = 0;
//     for (const Job& currentJob : sequence) {
//         sumP += currentJob.p;
//         if (currentJob.p > maxP) maxP = currentJob.p;
//         int C = sumP + (m - 1) * maxP;
//         totalCost += currentJob.w * C;
//     }
//     return totalCost;
// }

// // WSPT Sorting Rule
// bool compareWSPT(const Job& a, const Job& b) {
//     double ratioA = (double)a.w / a.p;
//     double ratioB = (double)b.w / b.p;
//     return ratioA > ratioB; 
// }

// int main() {
//     ifstream inputFile("jobs.txt"); // Open the text file
    
//     // Check if the file opened successfully
//     if (!inputFile.is_open()) {
//         cerr << "Error: Could not open jobs.txt. Make sure the file exists in the same folder!" << endl;
//         return 1;
//     }

//     int m;
//     inputFile >> m; // Read the first number as the number of machines
    
//     vector<Job> jobs;
//     Job tempJob;
    
//     // Loop through the rest of the file line by line
//     // It automatically stops when it runs out of data
//     while (inputFile >> tempJob.id >> tempJob.p >> tempJob.w) {
//         jobs.push_back(tempJob);
//     }
    
//     inputFile.close();
    
//     if (jobs.empty()) {
//         cerr << "Error: No jobs were read from the file." << endl;
//         return 1;
//     }

//     // Step 1: Re-index jobs according to WSPT
//     sort(jobs.begin(), jobs.end(), compareWSPT);

//     cout << "Loaded " << jobs.size() << " jobs for " << m << " machines.\n";
//     cout << "Sorted Order (WSPT): ";
//     for (const auto& job : jobs) cout << "J" << job.id << " ";
//     cout << "\n\n";

//     // Step 2 & 3: Minimum Cost Insertion Algorithm
//     vector<Job> optimalSequence;
//     optimalSequence.push_back(jobs[0]); 

//     for (size_t k = 1; k < jobs.size(); ++k) {
//         Job currentJob = jobs[k];
//         int bestCost = INT_MAX;
//         vector<Job> bestSequenceSoFar;
        
//         // --- THE LEMMA SHORTCUTS ---
//         vector<size_t> validPositions;
        
//         // The very end of the sequence is always a valid segment end (Lemma 6.3.3)
//         validPositions.push_back(optimalSequence.size());

//         int currentMaxP = -1;
//         for (size_t i = 0; i < optimalSequence.size(); ++i) {
//             // Identify a "new-max" job
//             if (optimalSequence[i].p > currentMaxP) {
//                 currentMaxP = optimalSequence[i].p;
                
//                 // Lemma 6.3.3: We can insert just before a new-max job
//                 // Lemma 6.3.2: But ONLY if placing it here doesn't put our currentJob
//                 // before a job that is faster than it.
//                 if (optimalSequence[i].p >= currentJob.p) {
//                     validPositions.push_back(i);
//                 }
//             }
//         }

//         // test ONLY the mathematically valid positions
//         for (size_t pos : validPositions) {
//             vector<Job> testSequence = optimalSequence;
//             testSequence.insert(testSequence.begin() + pos, currentJob);
            
//             int currentCost = calculateTotalCost(testSequence, m);
            
//             // Step 2 Rule: If costs are equal, choose the one inserted latest (<= ensures this)
//             if (currentCost <= bestCost) {
//                 bestCost = currentCost;
//                 bestSequenceSoFar = testSequence;
//             }
//         }
//         optimalSequence = bestSequenceSoFar; 
//     }

//     // Output the Final Result
//     cout << "--- FINAL OPTIMAL RESULT ---\n";
//     cout << "Sequence: ";
//     for (size_t i = 0; i < optimalSequence.size(); ++i) {
//         cout << "J" << optimalSequence[i].id;
//         if (i < optimalSequence.size() - 1) cout << " -> ";
//     }
    
//     cout << "\nTotal Minimized Cost: " << calculateTotalCost(optimalSequence, m) << endl;

//     return 0;
// }

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;
using Clock = chrono::steady_clock;
using Cost = __int128_t;

struct Job {
    int id{};
    long long p{};  // Processing time on every machine
    long long w{};  // Weight / priority
};

struct Solution {
    vector<Job> wsptOrder;
    vector<Job> optimalSequence;
    Cost totalCost{};
};

// Convert a signed 128-bit integer to text for safe output.
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

// Shakhlevich completion-time formula:
// C_j = prefixSumP + (m - 1) * prefixMaxP
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
// w_a / p_a > w_b / p_b  iff  w_a * p_b > w_b * p_a.
bool compareWSPT(const Job& a, const Job& b) {
    const Cost left = static_cast<Cost>(a.w) * b.p;
    const Cost right = static_cast<Cost>(b.w) * a.p;

    if (left != right) {
        return left > right;
    }

    // Deterministic tie-breaking for equal WSPT ratios.
    // Descending ID preserves the result produced by the original dataset/code.
    return a.id > b.id;
}

// Return the insertion positions allowed by the two dominance lemmas:
//   1. the end of the sequence;
//   2. immediately before a new-max job whose processing time is at least
//      the processing time of the job being inserted.
vector<size_t> findValidInsertionPositions(const vector<Job>& sequence,
                                           const Job& currentJob) {
    vector<size_t> validPositions;
    validPositions.reserve(sequence.size() + 1);

    // Insertion at the end is always considered.
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

Solution solveWSPTMCI(vector<Job> jobs, int m) {
    stable_sort(jobs.begin(), jobs.end(), compareWSPT);

    Solution solution;
    solution.wsptOrder = jobs;

    if (jobs.empty()) {
        return solution;
    }

    vector<Job> sequence;
    sequence.reserve(jobs.size());
    sequence.push_back(jobs.front());

    Cost currentTotalCost = calculateTotalCost(sequence, m);

    for (size_t jobIndex = 1; jobIndex < jobs.size(); ++jobIndex) {
        const Job& currentJob = jobs[jobIndex];
        const size_t k = sequence.size();

        // Prefix information for the existing sequence.
        // prefixSumP[r] and prefixMaxP[r] describe positions [0, r).
        vector<Cost> prefixSumP(k + 1, 0);
        vector<long long> prefixMaxP(k + 1, 0);

        for (size_t i = 0; i < k; ++i) {
            prefixSumP[i + 1] = prefixSumP[i] + sequence[i].p;
            prefixMaxP[i + 1] = max(prefixMaxP[i], sequence[i].p);
        }

        // Suffix information used to evaluate an insertion in O(1).
        // suffixWeight[r] = sum of weights at positions [r, k).
        // suffixMaxIncrease[r] stores the weighted increase in prefix maxima
        // caused by inserting currentJob before positions [r, k).
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
            // Completion time of the inserted job at this position.
            const Cost insertedCompletionTime =
                prefixSumP[position] + currentJob.p +
                static_cast<Cost>(m - 1) *
                    max(prefixMaxP[position], currentJob.p);

            // Every old job at or after 'position' gets p_current added to
            // its prefix sum. Its prefix maximum may also increase.
            const Cost candidateCost =
                currentTotalCost +
                static_cast<Cost>(currentJob.w) * insertedCompletionTime +
                static_cast<Cost>(currentJob.p) * suffixWeight[position] +
                static_cast<Cost>(m - 1) * suffixMaxIncrease[position];

            // If two positions have equal cost, choose the latest position.
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

        // Perform exactly one physical insertion after the best position is known.
        sequence.insert(sequence.begin() + static_cast<ptrdiff_t>(bestPosition),
                        currentJob);
        currentTotalCost = bestCost;
    }

    // Final consistency check. This is one O(n) scan and does not change the
    // overall O(n^2) complexity.
    const Cost verifiedCost = calculateTotalCost(sequence, m);
    if (verifiedCost != currentTotalCost) {
        throw logic_error("Internal cost-calculation mismatch.");
    }

    solution.optimalSequence = move(sequence);
    solution.totalCost = currentTotalCost;
    return solution;
}

int main(int argc, char* argv[]) {
    const string inputFileName = (argc >= 2) ? argv[1] : "jobs.txt";

    ifstream inputFile(inputFileName);
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open " << inputFileName << ".\n";
        return 1;
    }

    int m = 0;
    if (!(inputFile >> m) || m <= 0) {
        cerr << "Error: The first value must be a positive number of machines.\n";
        return 1;
    }

    vector<Job> jobs;
    unordered_set<int> seenIds;

    while (true) {
        Job job;

        if (!(inputFile >> job.id)) {
            break;
        }

        if (!(inputFile >> job.p >> job.w)) {
            cerr << "Error: Every job record must contain: <id> <p> <w>.\n";
            return 1;
        }

        if (job.p <= 0) {
            cerr << "Error: Job J" << job.id
                 << " has a non-positive processing time.\n";
            return 1;
        }

        if (job.w < 0) {
            cerr << "Error: Job J" << job.id << " has a negative weight.\n";
            return 1;
        }

        if (!seenIds.insert(job.id).second) {
            cerr << "Error: Duplicate job ID J" << job.id << ".\n";
            return 1;
        }

        jobs.push_back(job);
    }

    if (!inputFile.eof()) {
        cerr << "Error: Invalid data was found in " << inputFileName << ".\n";
        return 1;
    }

    if (jobs.empty()) {
        cerr << "Error: No jobs were read from " << inputFileName << ".\n";
        return 1;
    }

    try {
        const auto start = Clock::now();
        const Solution solution = solveWSPTMCI(jobs, m);
        const auto stop = Clock::now();

        const double elapsedMicroseconds =
            chrono::duration<double, micro>(stop - start).count();

        cout << "Loaded " << jobs.size() << " jobs for " << m
             << " machines.\n";

        cout << "WSPT Order: ";
        for (const Job& job : solution.wsptOrder) {
            cout << "J" << job.id << ' ';
        }
        cout << "\n\n";

        cout << "--- FINAL OPTIMAL RESULT ---\n";
        cout << "Sequence: ";
        for (size_t i = 0; i < solution.optimalSequence.size(); ++i) {
            cout << "J" << solution.optimalSequence[i].id;
            if (i + 1 < solution.optimalSequence.size()) {
                cout << " -> ";
            }
        }

        cout << "\nTotal Minimized Cost: "
             << costToString(solution.totalCost) << '\n';
        cout << "Execution Time: " << elapsedMicroseconds
             << " microseconds\n";
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;

}