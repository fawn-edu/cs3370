.POSIX:
.SUFFIXES:
.PHONY: all clean test

CXX = c++
CXXFLAGS = -std=c++23 -Wall -Wextra -pedantic -march=native

all: pulse pulse_debug gen_data
pulse: main.cpp pulse_config.cpp pulse_config.h
	$(CXX) $(CXXFLAGS) -O2 -o $@ main.cpp pulse_config.cpp
pulse_debug: main.cpp pulse_config.cpp pulse_config.h
	$(CXX) $(CXXFLAGS) -g -fsanitize=address,undefined -o $@ main.cpp pulse_config.cpp
gen_data: gen_data.cpp
	$(CXX) $(CXXFLAGS) -O2 -o $@ gen_data.cpp
clean:
	rm -f pulse pulse_debug gen_data *.exe *.pdb
