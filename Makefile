.POSIX:
.SUFFIXES:
.PHONY: clean test

CXX = c++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -pedantic -march=native

test: 3.out
	./3.out
3.out: main.cpp bigint.cpp bigint.h numeric.h perf_bigint.h test_bigint.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp bigint.cpp
clean:
	rm -f ?.out* ?.pdb
