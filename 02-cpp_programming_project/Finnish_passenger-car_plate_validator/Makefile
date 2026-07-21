# Finnish plate validator, C++17.
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude

all: plate_validator test_plate

plate_validator: src/main.cpp src/plate_validator.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

test_plate: tests/test_plate.cpp src/plate_validator.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

test: test_plate
	./test_plate

clean:
	rm -f plate_validator test_plate

.PHONY: all test clean
