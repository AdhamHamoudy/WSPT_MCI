#!/usr/bin/env bash
set -euo pipefail

ITERATIONS="${1:-1000}"
SEED="${2:-1783029008571364603}"
MAX_JOBS="${3:-11}"

echo "============================================================"
echo " PART I BENCHMARK: WSPT-MCI DIFFERENTIAL TEST"
echo "============================================================"
echo "Compiler:   $(g++ --version | head -n 1)"
echo "System:     $(uname -srmo)"
echo "Iterations: ${ITERATIONS}"
echo "Seed:       ${SEED}"
echo "Job range:  2..${MAX_JOBS}"
echo
echo "Algorithms compared:"
echo "  1. True permutation brute force"
echo "  2. WSPT + all-position MCI"
echo "  3. Optimized WSPT-MCI"
echo "============================================================"
echo

echo "Compiling fuzzer_part1..."
make fuzzer_part1
echo

echo "Running Part I randomized differential benchmark..."
time ./fuzzer_part1 "${ITERATIONS}" "${SEED}" "${MAX_JOBS}"

echo
echo "============================================================"
echo " PART I BENCHMARK COMPLETE"
echo "============================================================"
