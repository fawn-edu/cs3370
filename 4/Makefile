.POSIX:
.SUFFIXES:
.PHONY: all clean test

CXX = c++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -pedantic -march=native

test: tests
	./tests
all: tests config
tests: test_config.cpp pulse_config.cpp pulse_config.h
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined -o $@ test_config.cpp pulse_config.cpp
config: main.cpp pulse_config.cpp pulse_config.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp pulse_config.cpp
clean:
	rm -f tests* config* *.pdb
