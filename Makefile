.POSIX:
.SUFFIXES:
.PHONY: clean test

CXX = c++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -pedantic -march=native -fsanitize=address,undefined

test: pulse
	./pulse gage2scope.ini
pulse: main.cpp pulse_config.cpp pulse_config.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp pulse_config.cpp
clean:
	rm -f pulse pulse.exe *.pdb
