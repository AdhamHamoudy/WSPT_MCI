#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

struct Job {
    int id;
    int p;
    int w;
    int e; 
};

// The State struct will hold the memory for our DP grid
struct DPState {
    int minCost = INT_MAX; 
    vector<Job> keptJobs;
};

// --- CORE MATHEMATICS ---
int calculateTotalCost(const vector<Job>& sequence, int m) {
    if (sequence.empty()) return 0;
    
    int totalCost = 0;
    int sumP = 0;
    int maxP = 0;
    for (const Job& currentJob : sequence) {
        sumP += currentJob.p;
        if (currentJob.p > maxP) maxP = currentJob.p;
        int C = sumP + (m - 1) * maxP;
        totalCost += currentJob.w * C;
    }
    return totalCost;
}

// Pre-sort condition: WSPT
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

    cout << "Loaded " << n << " jobs for " << m << " machines.\n";
    cout << "Maximum Rejection Budget (U): " << U << "\n";
    cout << "Initializing DP State Machine (O(n * U))...\n";
    cout << "----------------------------------------\n";

    auto startDP = high_resolution_clock::now();

    // --- STEP 1: PRE-SORT BY WSPT ---
    // If the master list is sorted, any subset we pull will naturally be sorted optimally.
    sort(allJobs.begin(), allJobs.end(), compareWSPT);

    // --- STEP 2: BUILD THE DP GRID ---
    // dp[i][b] = DPState storing the min cost and sequence for first 'i' jobs using exactly 'b' budget
    vector<vector<DPState>> dp(n + 1, vector<DPState>(U + 1));
    
    // Base Case: 0 jobs evaluated, 0 budget spent = 0 cost
    dp[0][0].minCost = 0;

    // --- STEP 3: EXECUTE STATE MACHINE ---
    for (int i = 1; i <= n; ++i) {
        Job currentJob = allJobs[i - 1]; // The job we are currently deciding on
        
        for (int b = 0; b <= U; ++b) {
            // If this state was never reached, skip it
            if (dp[i - 1][b].minCost == INT_MAX) continue;

            // PATH A: KEEP THE JOB
            // Budget 'b' stays the same. We append the job and calculate new cost.
            vector<Job> keptPath = dp[i - 1][b].keptJobs;
            keptPath.push_back(currentJob);
            int keptCost = calculateTotalCost(keptPath, m);

            if (keptCost < dp[i][b].minCost) {
                dp[i][b].minCost = keptCost;
                dp[i][b].keptJobs = keptPath;
            }

            // PATH B: REJECT THE JOB
            // Budget increases by e. The scheduling cost does not go up.
            int next_b = b + currentJob.e;
            if (next_b <= U) { // Only take this path if we don't blow the budget
                if (dp[i - 1][b].minCost < dp[i][next_b].minCost) {
                    dp[i][next_b].minCost = dp[i - 1][b].minCost;
                    dp[i][next_b].keptJobs = dp[i - 1][b].keptJobs;
                }
            }
        }
    }

    // --- STEP 4: FIND THE ABSOLUTE BEST OUTCOME ---
    // Look across the final row (all jobs evaluated) to find the cheapest valid budget configuration
    int globalMinCost = INT_MAX;
    int bestBudgetSpent = 0;
    vector<Job> optimalSubset;

    for (int b = 0; b <= U; ++b) {
        if (dp[n][b].minCost < globalMinCost) {
            globalMinCost = dp[n][b].minCost;
            bestBudgetSpent = b;
            optimalSubset = dp[n][b].keptJobs;
        }
    }

    auto stopDP = high_resolution_clock::now();
    auto durationDP = duration_cast<microseconds>(stopDP - startDP);

    // --- OUTPUT RESULTS ---
    cout << "[OPTIMAL SUBSET FOUND VIA DP]\n";
    cout << "Total Rejection Cost Spent: " << bestBudgetSpent << " / " << U << "\n";
    cout << "Total Minimized Schedule Cost: " << globalMinCost << "\n";
    cout << "Kept Jobs: ";
    for (const Job& j : optimalSubset) {
        cout << "J" << j.id << " ";
    }
    cout << "\n\n";
    cout << "DP Execution Time: " << durationDP.count() << " microseconds\n";
    cout << "========================================\n";

    return 0;
}