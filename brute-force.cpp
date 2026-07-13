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
};

// --- CORE MATHEMATICS ---
int calculateTotalCost(const vector<Job>& sequence, int m) {
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

bool compareWSPT(const Job& a, const Job& b) {
    double ratioA = (double)a.w / a.p;
    double ratioB = (double)b.w / b.p;
    return ratioA > ratioB; 
}

// --- ALGORITHM 1: PURE BRUTE FORCE O(N!) ---
int solveBruteForceFactorial(vector<Job> jobs, int m, vector<Job>& bestSequenceOut) {
    // To generate ALL permutations, the array must be sorted by ID first
    sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.id < b.id;
    });

    int minCost = INT_MAX;

    // std::next_permutation generates every single unique order O(N!)
    do {
        int currentCost = calculateTotalCost(jobs, m);
        if (currentCost < minCost) {
            minCost = currentCost;
            bestSequenceOut = jobs;
        }
    } while (next_permutation(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.id < b.id;
    }));

    return minCost;
}

// --- ALGORITHM 2: OPTIMIZED WSPT-MCI O(N^2) ---
int solveWSPTMCI(vector<Job> jobs, int m, vector<Job>& bestSequenceOut) {
    // Step 1: Sort by WSPT
    sort(jobs.begin(), jobs.end(), compareWSPT);

    vector<Job> optimalSequence;
    optimalSequence.push_back(jobs[0]); 

    // Step 2 & 3: Minimum Cost Insertion with Lemma Filtering
    for (size_t k = 1; k < jobs.size(); ++k) {
        Job currentJob = jobs[k];
        int bestCost = INT_MAX;
        vector<Job> bestSequenceSoFar;
        
        vector<size_t> validPositions;
        validPositions.push_back(optimalSequence.size()); // End of sequence

        int currentMaxP = -1;
        for (size_t i = 0; i < optimalSequence.size(); ++i) {
            if (optimalSequence[i].p > currentMaxP) {
                currentMaxP = optimalSequence[i].p;
                if (optimalSequence[i].p >= currentJob.p) { // Lemma 6.3.2 & 6.3.3
                    validPositions.push_back(i);
                }
            }
        }

        for (size_t pos : validPositions) {
            vector<Job> testSequence = optimalSequence;
            testSequence.insert(testSequence.begin() + pos, currentJob);
            
            int currentCost = calculateTotalCost(testSequence, m);
            if (currentCost <= bestCost) {
                bestCost = currentCost;
                bestSequenceSoFar = testSequence;
            }
        }
        optimalSequence = bestSequenceSoFar; 
    }
    
    bestSequenceOut = optimalSequence;
    return calculateTotalCost(optimalSequence, m);
}

// --- MAIN EXECUTION ---
int main() {
    ifstream inputFile("jobs.txt");
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open jobs.txt" << endl;
        return 1;
    }

    int m;
    inputFile >> m;
    vector<Job> jobs;
    Job tempJob;
    while (inputFile >> tempJob.id >> tempJob.p >> tempJob.w) {
        jobs.push_back(tempJob);
    }
    inputFile.close();

    cout << "Loaded " << jobs.size() << " jobs for " << m << " machines.\n";
    cout << "----------------------------------------\n";

    // Variables to hold the winning sequences
    vector<Job> bruteForceSeq, optimizedSeq;

    // --- BENCHMARK: OPTIMIZED O(N^2) ---
    auto startOpt = high_resolution_clock::now();
    int costOpt = solveWSPTMCI(jobs, m, optimizedSeq);
    auto stopOpt = high_resolution_clock::now();
    auto durationOpt = duration_cast<microseconds>(stopOpt - startOpt);

    cout << "[WSPT-MCI O(n^2)]\n";
    cout << "Minimum Cost: " << costOpt << "\n";
    cout << "Execution Time: " << durationOpt.count() << " microseconds\n\n";

    // --- BENCHMARK: BRUTE FORCE O(N!) ---
    cout << "Running Brute Force O(n!)... (This may take a moment)\n";
    auto startBrute = high_resolution_clock::now();
    int costBrute = solveBruteForceFactorial(jobs, m, bruteForceSeq);
    auto stopBrute = high_resolution_clock::now();
    auto durationBrute = duration_cast<microseconds>(stopBrute - startBrute);

    cout << "[Brute Force O(n!)]\n";
    cout << "Minimum Cost: " << costBrute << "\n";
    cout << "Execution Time: " << durationBrute.count() << " microseconds\n\n";

    // --- VERIFICATION ---
    cout << "----------------------------------------\n";
    if (costOpt == costBrute) {
        cout << "SUCCESS: Both algorithms found the exact same optimal cost!\n";
    } else {
        cout << "WARNING: Cost mismatch! Something is wrong.\n";
    }

    return 0;
}