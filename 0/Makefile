.POSIX:
.SUFFIXES:
.PHONY: clean run

CXX = c++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -pedantic

run: 0.out
	./0.out
0.out: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ main.cpp
main.cpp: rectangle.hpp test_harness.hpp frustum.hpp
clean:
	rm -f ?.out*
