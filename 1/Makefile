.POSIX:
.SUFFIXES:
.PHONY: clean test

CXX = c++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -pedantic

test: 1.out
	./1.out
1.out: CircBuf.cpp main.cpp
	$(CXX) $(CXXFLAGS) -o $@ CircBuf.cpp main.cpp
main.cpp: CircBuf.h
CircBuf.cpp: CircBuf.h
clean:
	rm -f ?.out*
