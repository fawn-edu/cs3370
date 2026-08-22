// CircBuf.cpp
// Fawn Sannar <10725695@uvu.edu>

#include <algorithm>
#include "CircBuf.h"

using std::string;

CircBuf::CircBuf(size_t reserve) {
	cap = chunks_needed(reserve) * CHUNK;
	buf = new char[cap];
}

CircBuf::~CircBuf() { delete[] buf; }

size_t CircBuf::size() const { return head - tail; }

size_t CircBuf::capacity() const { return cap; }

void CircBuf::insert(char c) { insert(&c, 1); }

void CircBuf::insert(const char *s, size_t sz) {
	const auto newsize = size() + sz;
	if (newsize > cap) resize(newsize);
	const auto h = head % cap;
	head += sz;
	const auto fits = std::min(cap - h, sz);
	// Copy into free space starting from head and ending at the end of
	// the buffer or after the input is exhausted, whichever is first
	std::copy_n(s, fits, &buf[h]);
	// Copy the remainder of the input, if any
	std::copy_n(&s[fits], sz - fits, buf);
}

void CircBuf::insert(const string &s) { insert(s.data(), s.length()); }

// The assignment doesn't specify what to do if the buffer is empty, so
// in that case get() returns a null byte
char CircBuf::get() { return tail < head ? buf[tail++ % cap] : '\0'; }

string CircBuf::get(size_t cnt) {
	cnt = std::min(cnt, size());
	const auto t = tail % cap;
	tail += cnt;
	const auto fits = std::min(cap - t, cnt);
	// Construct string from data starting from tail and ending at the
	// end of the buffer or at head, whichever is first, followed by any
	// remaining data beginning at the start of the buffer until head
	return string(&buf[t], fits) + string(buf, cnt - fits);
}

string CircBuf::flush() { return get(size()); }

string CircBuf::examine() const {
	auto s = string(cap, '-');
	// Replace '-' with data from buffer, where it is present
	if (size()) {
		const auto t = tail % cap;
		const auto fits = std::min(cap - t, size());
		// Copy from buffer starting from tail and ending at the
		// end of the buffer or at head, whichever is first
		std::copy_n(&buf[t], fits, &s[t]);
		// Copy the remainder of the buffer, if any
		std::copy_n(buf, size() - fits, s.data());
	}
	return '[' + s + ']';
}

void CircBuf::shrink() { resize(size()); }

constexpr size_t CircBuf::chunks_needed(size_t cnt) {
	return (cnt + CHUNK - 1) / CHUNK;
}

void CircBuf::resize(size_t desired) {
	const auto chunks_desired = chunks_needed(desired);
	const auto chunks_actual = chunks_needed(cap);
	if (chunks_actual == chunks_desired) return;
	const auto newcap = chunks_desired * CHUNK;
	auto *const newbuf = new char[newcap];

	if (size()) {
		const auto t = tail % cap;
		const auto fits = std::min(cap - t, size());
		// Copy from old buffer starting from tail and ending at the
		// end of the old buffer or at head, whichever is first
		std::copy_n(&buf[t], fits, newbuf);
		// Copy the remainder of the old buffer, if any
		std::copy_n(buf, size() - fits, &newbuf[fits]);
	}

	delete[] buf;
	cap = newcap;
	head = size();
	tail = 0;
	buf = newbuf;
}
