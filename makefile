# ============================================================
# Compiler settings
# ============================================================

CXX := g++
CXXFLAGS := -O3 -Wall -Wextra -std=c++17


# ============================================================
# Executables grouped by project part
# ============================================================

PART1_TARGETS := WSPT-MCI unoptimized_wspt brute-force fuzzer_part1
PART2_TARGETS := dp_with_budget naive_with_budget fuzzer_part2
ALL_TARGETS := $(PART1_TARGETS) $(PART2_TARGETS)


# ============================================================
# Benchmark defaults
#
# Examples:
#   make benchmark-part1
#   make benchmark-part1 ITERATIONS=1000 SEED=1783029008571364603 MAX_JOBS=11
#   make benchmark-part2 ITERATIONS=1000 SEED=1783029008571364603
# ============================================================

ITERATIONS ?= 1000
SEED ?= 1783029008571364603
MAX_JOBS ?= 11


# ============================================================
# Default build
# ============================================================

all: $(ALL_TARGETS)


# ============================================================
# Part I build rules
# ============================================================

WSPT-MCI: WSPT-MCI.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

unoptimized_wspt: unoptimized_wspt.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

brute-force: brute-force.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

fuzzer_part1: fuzzer_part1.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<


# ============================================================
# Part II build rules
# ============================================================

dp_with_budget: dp_with_budget.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

naive_with_budget: naive_with_budget.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

fuzzer_part2: fuzzer_part2.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<


# ============================================================
# Run individual solvers
# ============================================================

run-part1: WSPT-MCI
	./WSPT-MCI

run-dp: dp_with_budget
	./dp_with_budget

run-naive-budget: naive_with_budget
	./naive_with_budget


# ============================================================
# Randomized differential benchmarks
# ============================================================

benchmark-part1: fuzzer_part1
	./fuzzer_part1 $(ITERATIONS) $(SEED) $(MAX_JOBS)

benchmark-part2: fuzzer_part2
	./fuzzer_part2 $(ITERATIONS) $(SEED)


# ============================================================
# Clean compiled executables
# ============================================================

clean:
	rm -f $(ALL_TARGETS)


.PHONY: all \
	run-part1 run-dp run-naive-budget \
	benchmark-part1 benchmark-part2 \
	clean
