// Pool.hpp
// Fawn Sannar <10725695@uvu.edu>
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio> // <print> requires GCC 14+, Github codespaces use 13.3
#include <new>
#include <span>
#include <stdexcept>
#include <vector>

template<class T>
class Pool {
public:
	Pool(std::size_t count, bool trace = true)
		: data{new(std::align_val_t{alignof(T)}) std::byte[count * sizeof(T)], count * sizeof(T)}
		, trace_enabled{trace}
	{
		for (auto i = 1u; i <= count; ++i) freelist.emplace_back(count - i);
	}
	~Pool() { ::operator delete[](data.data(), std::align_val_t{alignof(T)}); } // disgusting

	[[nodiscard]] T *allocate() {
		if (freelist.empty()) throw std::bad_alloc{};
		const auto i = freelist.back(); // 🪦 Here lies one hour of my time
		freelist.pop_back();
		if (trace_enabled) printf("alloc @ %zu (%p)\n", i, &data[i]);
		return reinterpret_cast<T*>(data.data()) + i;
	}

	void deallocate(T *ptr) {
		// Convert the pointer into its corresponding index in data
		const std::ptrdiff_t i = ptr - reinterpret_cast<T*>(data.data());
		// Make sure it's actually a valid index into data and not some evil arbitrary pointer
		if (i < 0 || static_cast<std::size_t>(i) > data.size() / sizeof(T)) throw std::out_of_range{"ptr"};
		// No double-frees allowed!!
		if (std::ranges::find(freelist, i) != freelist.end()) throw std::invalid_argument{"ptr"};
		if (trace_enabled) printf("dealloc @ %zu (%p)\n", static_cast<std::size_t>(i), &data[i]);
		freelist.emplace_back(i);
	}

	void profile() const {
		printf(
			"live=%zu, free=%zu, freelist=[",
			data.size() / sizeof(T) - freelist.size(), // No need to store live npc count since it is trivially computable
			freelist.size()
		);
		for (auto i = 0; i < freelist.size(); ++i) printf("%zu%s", i, i == freelist.size() - 1 ? "]\n" : ", ");
	}
private:
	std::span<std::byte> data;
	std::vector<std::size_t> freelist;
	bool trace_enabled;
};
