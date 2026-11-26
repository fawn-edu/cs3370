.POSIX:
.SUFFIXES:
.PHONY: clean test

CXX = c++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -pedantic -march=native

test: 7.out
	rm -f region_?.txt pipeline.log
	./7.out
7.out: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ main.cpp
clean:
	rm -f ?.out* *.pdb region_?.txt pipeline.log
