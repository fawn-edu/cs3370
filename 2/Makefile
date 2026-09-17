.POSIX:
.SUFFIXES:
.PHONY: clean test

CXX = c++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -pedantic

test: 2.out
	./2.out
2.out: NPC.cpp main.cpp assignment2_test_harness.hpp NPC.hpp Pool.hpp
	$(CXX) $(CXXFLAGS) -o $@ NPC.cpp main.cpp
clean:
	rm -f ?.out*
