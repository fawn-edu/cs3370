// Pool.hpp
// Fawn Sannar <10725695@uvu.edu>
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>
#include <print> // Apparently std::print() and family refuse to print pointers, hence the below reinterpret_casts to uintptr_t
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
		if (trace_enabled) std::println("alloc @ {} ({:#x})", i, reinterpret_cast<std::uintptr_t>(&data[i]));
		return reinterpret_cast<T*>(data.data()) + i;
	}

	void deallocate(T *ptr) {
		// Convert the pointer into its corresponding index in data
		const std::ptrdiff_t i = ptr - reinterpret_cast<T*>(data.data());
		// Make sure it's actually a valid index into data and not some evil arbitrary pointer
		if (i < 0 || static_cast<std::size_t>(i) > data.size() / sizeof(T)) throw std::out_of_range{"ptr"};
		// No double-frees allowed!!
		if (std::ranges::find(freelist, i) != freelist.end()) throw std::invalid_argument{"ptr"};
		if (trace_enabled) std::println("dealloc @ {} ({:#x})", i, reinterpret_cast<std::uintptr_t>(&data[i]));
		freelist.emplace_back(i);
	}

	void profile() const {
		std::println(
			"live={}, free={}, freelist={}",
			data.size() / sizeof(T) - freelist.size(), // No need to store live npc count since it is trivially computable
			freelist.size(),
			freelist
		);
	}
private:
	std::span<std::byte> data;
	std::vector<std::size_t> freelist;
	bool trace_enabled;
};
