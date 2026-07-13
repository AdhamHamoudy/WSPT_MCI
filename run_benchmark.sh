#!/bin/bash

# Clear the screen for a clean output
clear

echo "========================================"
echo "   SCHEDULING ALGORITHM BENCHMARKING    "
echo "========================================"
echo ""

# Compile everything using your Makefile
echo "-> Compiling source files..."
make
echo "Compilation complete."
echo ""

# Run the O(n^2) Optimized Algorithm
echo "----------------------------------------"
echo "1. Running Optimized WSPT-MCI (O(n^2))"
echo "----------------------------------------"
time ./WSPT-MCI
echo ""

# Run the O(n^3) Unoptimized Algorithm
echo "----------------------------------------"
echo "2. Running Unoptimized WSPT (O(n^3))"
echo "----------------------------------------"
time ./unoptimized_wspt
echo ""

# Run the O(n!) Brute Force Algorithm
echo "----------------------------------------"
echo "3. Running Pure Brute Force (O(n!))"
echo "----------------------------------------"
time ./brute-force
echo ""

echo "========================================"
echo "          BENCHMARK COMPLETE            "
echo "========================================"