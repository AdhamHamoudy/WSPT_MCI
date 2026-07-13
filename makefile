# ============================================================
# Compiler settings
# ============================================================

CXX = g++
CXXFLAGS = -O3 -Wall -Wextra -std=c++17


# ============================================================
# Executable targets
# ============================================================

TARGET1 = brute-force
TARGET2 = unoptimized_wspt
TARGET3 = WSPT-MCI
TARGET4 = dp_with_budget
TARGET5 = fuzzer
TARGET6 = naive_with_budget


# ============================================================
# Source files
# ============================================================

SRC1 = brute-force.cpp
SRC2 = unoptimized_wspt.cpp
SRC3 = WSPT-MCI.cpp
SRC4 = dp_with_budget.cpp
SRC5 = fuzzer.cpp
SRC6 = naive_with_budget.cpp


# ============================================================
# Fuzzer settings
#
# Examples:
#   make benchmark
#   make benchmark ITERATIONS=5000
#   make benchmark ITERATIONS=1000 SEED=1783029008571364603
# ============================================================

ITERATIONS ?= 1000
SEED ?=


# ============================================================
# Default target
# ============================================================

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6)


# ============================================================
# Build rules
# ============================================================

$(TARGET1): $(SRC1)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET2): $(SRC2)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET3): $(SRC3)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET4): $(SRC4)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET5): $(SRC5)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET6): $(SRC6)
	$(CXX) $(CXXFLAGS) -o $@ $<


# ============================================================
# Run the timed fuzzer
# ============================================================

benchmark: $(TARGET5)
	@if [ -n "$(SEED)" ]; then \
		time ./$(TARGET5) $(ITERATIONS) $(SEED); \
	else \
		time ./$(TARGET5) $(ITERATIONS); \
	fi


# Run only the exact DP solver
run-dp: $(TARGET4)
	./$(TARGET4)


# Run the naive budget solver
run-naive-budget: $(TARGET6)
	./$(TARGET6)


# ============================================================
# Clean compiled executables
# ============================================================

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6)


.PHONY: all benchmark run-dp run-naive-budget clean