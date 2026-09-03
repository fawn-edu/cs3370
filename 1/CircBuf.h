// CircBuf.h
// Fawn Sannar <10725695@uvu.edu>
// I would rename this file to "CircBuf.hpp" since I like to make it
// obvious when a header file is not C, but that would *technically*
// require me to modify main.cpp, which is forbidden. Would it probably
// be fine? Yeah, but I've also been a TA before, and I wouldn't be
// surprised if this was enforced during grading by comparing md5 hashes
// or something and I'm not taking any chances lol
#pragma once

#include <cstddef>
#include <string>

class CircBuf {
public:
	CircBuf(size_t reserve = 0); // Number of elements you want it to be able to hold to start with.
	~CircBuf();
	size_t size() const; // Get the number of items in the buffer
	size_t capacity() const; // Get the current capacity of the buffer
	void insert(char c); // Put a single char into the buffer
	void insert(const char *s, size_t sz); // Put several chars into the buffer
	void insert(const std::string &s); // Put all chars in a string into the buffer
	char get(); // Get a single char from the buffer
	std::string get(size_t cnt); // Get several chars from the bufffer as a string
	std::string flush(); // Returns a string with all the characters, AND shrinks the buffer to zero.
	std::string examine() const; // Returns a string representing the internal state of the buffer
	void shrink(); // Reduces the unused space in the buffer.
private:
	static constexpr size_t chunks_needed(size_t cnt); // Get the minimum number of chunks required to hold cnt items
	void resize(size_t desired); // Resize the backing array such that it can fit desired number of chars
	static constexpr size_t CHUNK = 8; // Minimum allocation unit

	// Both head and tail must be modulo'd by cap to get the actual
	// indicies they represent. This makes it possible to distinguish an
	// empty buffer from a full one without evil hacks.
	// This technically fails when cap == SIZE_MAX, but the creation of
	// such a buffer is itself a fail state. Most machines don't have
	// that much memory, and if they do, such a buffer cannot coexist in
	// memory with the program constructing it.
	size_t cap = 0, head = 0, tail = 0;
	char *buf = nullptr; // Backing dynamic array
};
