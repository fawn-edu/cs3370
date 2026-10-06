.POSIX:
.SUFFIXES:
.PHONY: clean test

CXX = c++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -pedantic

test: 3.out
	./3.out
3.out: main.cpp bigint.cpp bigint.hpp numeric.hpp perf_bigint.hpp test_bigint.hpp
	$(CXX) $(CXXFLAGS) -o $@ main.cpp bigint.cpp
clean:
	rm -f ?.out*
