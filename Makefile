# Compiler settings - Can change to clang++ if preferred
CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread

# Build targets
TARGET = test
SRC = thread-pool.cc tptest.cc Semaphore.cc

# Link the target with object files
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
