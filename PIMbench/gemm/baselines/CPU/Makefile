# Compiler
CXX := g++

# Compiler flags
CXXFLAGS := -Wall -Wextra -Werror -march=native -std=c++17 -O3 -fopenmp
LDFLAGS = -lopenblas

# Executable name
EXEC := gemm.out

# Source files
SRC_FILES := $(wildcard *.cpp)

# Dependancy
DEP := ../../../../util/

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(SRC_FILES) |
	$(CXX) $(CXXFLAGS) -I$(DEP) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(EXEC)
	