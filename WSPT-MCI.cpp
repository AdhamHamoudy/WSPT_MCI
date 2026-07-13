#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <fstream>

using namespace std;

struct Job {
    int id;
    int p;
    int w;
};

// Shakhlevich Formula Function
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

// WSPT Sorting Rule
bool compareWSPT(const Job& a, const Job& b) {
    double ratioA = (double)a.w / a.p;
    double ratioB = (double)b.w / b.p;
    return ratioA > ratioB; 
}

int main() {
    ifstream inputFile("jobs.txt"); // Open the text file
    
    // Check if the file opened successfully
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open jobs.txt. Make sure the file exists in the same folder!" << endl;
        return 1;
    }

    int m;
    inputFile >> m; // Read the first number as the number of machines
    
    vector<Job> jobs;
    Job tempJob;
    
    // Loop through the rest of the file line by line
    // It automatically stops when it runs out of data
    while (inputFile >> tempJob.id >> tempJob.p >> tempJob.w) {
        jobs.push_back(tempJob);
    }
    
    inputFile.close();
    
    if (jobs.empty()) {
        cerr << "Error: No jobs were read from the file." << endl;
        return 1;
    }

    // Step 1: Re-index jobs according to WSPT
    sort(jobs.begin(), jobs.end(), compareWSPT);

    cout << "Loaded " << jobs.size() << " jobs for " << m << " machines.\n";
    cout << "Sorted Order (WSPT): ";
    for (const auto& job : jobs) cout << "J" << job.id << " ";
    cout << "\n\n";

    // Step 2 & 3: Minimum Cost Insertion Algorithm
    vector<Job> optimalSequence;
    optimalSequence.push_back(jobs[0]); 

    for (size_t k = 1; k < jobs.size(); ++k) {
        Job currentJob = jobs[k];
        int bestCost = INT_MAX;
        vector<Job> bestSequenceSoFar;
        
        // --- THE LEMMA SHORTCUTS ---
        vector<size_t> validPositions;
        
        // The very end of the sequence is always a valid segment end (Lemma 6.3.3)
        validPositions.push_back(optimalSequence.size());

        int currentMaxP = -1;
        for (size_t i = 0; i < optimalSequence.size(); ++i) {
            // Identify a "new-max" job
            if (optimalSequence[i].p > currentMaxP) {
                currentMaxP = optimalSequence[i].p;
                
                // Lemma 6.3.3: We can insert just before a new-max job
                // Lemma 6.3.2: But ONLY if placing it here doesn't put our currentJob
                // before a job that is faster than it.
                if (optimalSequence[i].p >= currentJob.p) {
                    validPositions.push_back(i);
                }
            }
        }

        // test ONLY the mathematically valid positions
        for (size_t pos : validPositions) {
            vector<Job> testSequence = optimalSequence;
            testSequence.insert(testSequence.begin() + pos, currentJob);
            
            int currentCost = calculateTotalCost(testSequence, m);
            
            // Step 2 Rule: If costs are equal, choose the one inserted latest (<= ensures this)
            if (currentCost <= bestCost) {
                bestCost = currentCost;
                bestSequenceSoFar = testSequence;
            }
        }
        optimalSequence = bestSequenceSoFar; 
    }

    // Output the Final Result
    cout << "--- FINAL OPTIMAL RESULT ---\n";
    cout << "Sequence: ";
    for (size_t i = 0; i < optimalSequence.size(); ++i) {
        cout << "J" << optimalSequence[i].id;
        if (i < optimalSequence.size() - 1) cout << " -> ";
    }
    
    cout << "\nTotal Minimized Cost: " << calculateTotalCost(optimalSequence, m) << endl;

    return 0;
}