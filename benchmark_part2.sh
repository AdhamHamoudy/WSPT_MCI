#!/usr/bin/env bash
set -euo pipefail

ITERATIONS="${1:-1000}"
SEED="${2:-1783029008571364603}"

echo "============================================================"
echo " PART II BENCHMARK: JOB-REJECTION DIFFERENTIAL TEST"
echo "============================================================"
echo "Compiler:   $(g++ --version | head -n 1)"
echo "System:     $(uname -srmo)"
echo "Iterations: ${ITERATIONS}"
echo "Seed:       ${SEED}"
echo "Jobs/test:  8"
echo
echo "Algorithms compared:"
echo "  1. True brute force over subsets and permutations"
echo "  2. Exact subset DP"
echo "============================================================"
echo

echo "Compiling fuzzer_part2..."
make fuzzer_part2
echo

echo "Running Part II randomized differential benchmark..."
time ./fuzzer_part2 "${ITERATIONS}" "${SEED}"

echo
echo "============================================================"
echo " PART II BENCHMARK COMPLETE"
echo "============================================================"
