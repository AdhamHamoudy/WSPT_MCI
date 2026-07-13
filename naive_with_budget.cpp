#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

// Updated Struct to include Rejection Cost (e)
struct Job {
    int id;
    int p;
    int w;
    int e; 
};

// --- CORE MATHEMATICS ---
// Upgraded to 'long long' to prevent overflow on massive final costs
long long calculateTotalCost(const vector<Job>& sequence, int m) {
    if (sequence.empty()) return 0;
    
    long long totalCost = 0;
    long long sumP = 0;
    long long maxP = 0;
    
    for (const Job& currentJob : sequence) {
        sumP += currentJob.p;
        if (currentJob.p > maxP) maxP = currentJob.p;
        long long C = sumP + (m - 1) * maxP;
        totalCost += currentJob.w * C;
    }
    return totalCost;
}

bool compareWSPT(const Job& a, const Job& b) {
    double ratioA = (double)a.w / a.p;
    double ratioB = (double)b.w / b.p;
    return ratioA > ratioB; 
}

// --- MAIN EXECUTION ---
int main() {
    ifstream inputFile("jobs_with_budget.txt");
    
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open jobs_with_budget.txt!" << endl;
        return 1;
    }

    int m, U;
    inputFile >> m >> U; 
    
    vector<Job> allJobs;
    Job tempJob;
    
    while (inputFile >> tempJob.id >> tempJob.p >> tempJob.w >> tempJob.e) {
        allJobs.push_back(tempJob);
    }
    inputFile.close();

    int n = allJobs.size();
    
    // 1LL ensures the bit shift uses a 64-bit integer to prevent overflow
    long long totalCombinations = 1LL << n; 

    cout << "Loaded " << n << " jobs for " << m << " machines.\n";
    cout << "Maximum Rejection Budget (U): " << U << "\n";
    cout << "Analyzing " << totalCombinations << " combinations (O(2^n))...\n";
    cout << "----------------------------------------\n";

    long long globalMinCost = LLONG_MAX; 
    vector<Job> bestSubset;
    int bestRejectionCost = 0;

    // --- START THE STOPWATCH ---
    auto startTimer = high_resolution_clock::now();

    // --- BITMASKING COMBINATION GENERATOR ---
    for (long long mask = 0; mask < totalCombinations; ++mask) {
        vector<Job> keptJobs;
        int currentRejectionCost = 0;

        // Check each bit in the current mask
        for (int i = 0; i < n; ++i) {
            // Use 1LL to ensure the bitwise check matches the 64-bit mask
            if ((mask & (1LL << i)) != 0) {
                keptJobs.push_back(allJobs[i]);
            } else {
                currentRejectionCost += allJobs[i].e;
            }
        }

        // --- BUDGET FILTER & SCHEDULING ---
        // Only proceed if we haven't blown the budget U
        if (currentRejectionCost <= U) {
            
            sort(keptJobs.begin(), keptJobs.end(), compareWSPT);
            
            long long currentScheduleCost = calculateTotalCost(keptJobs, m);
            
            if (currentScheduleCost < globalMinCost) {
                globalMinCost = currentScheduleCost;
                bestSubset = keptJobs;
                bestRejectionCost = currentRejectionCost;
            }
        }
    }

    // --- STOP THE STOPWATCH ---
    auto stopTimer = high_resolution_clock::now();
    auto durationSec = duration_cast<seconds>(stopTimer - startTimer);
    auto durationMs = duration_cast<milliseconds>(stopTimer - startTimer);

    // --- OUTPUT RESULTS ---
    cout << "[OPTIMAL SUBSET FOUND]\n";
    cout << "Total Rejection Cost Spent: " << bestRejectionCost << " / " << U << "\n";
    cout << "Total Minimized Schedule Cost: " << globalMinCost << "\n";
    cout << "Kept Jobs: ";
    for (const Job& j : bestSubset) {
        cout << "J" << j.id << " ";
    }
    cout << "\n\n";
    cout << "Execution Time: " << durationSec.count() << " seconds (" << durationMs.count() << " ms)\n";
    cout << "========================================\n";

    return 0;
}