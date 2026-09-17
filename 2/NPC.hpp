// NPC.hpp
// Fawn Sannar <10725695@uvu.edu>
#pragma once

#include <cstddef>
#include <string>
#include <ostream>
#include "Pool.hpp"

class NPC {
public:
	NPC() = delete;
	NPC(const NPC&) = delete;
	NPC(NPC&&) = delete;
	NPC &operator=(const NPC&) = delete;
	NPC &operator=(NPC&&) = delete;

	static NPC *spawn(const std::string &name, const std::string &type, int x, int y, int health = 100);
	static void profile();

	friend std::ostream &operator<<(std::ostream &s, const NPC &npc);
	static void *operator new(std::size_t);
	static void operator delete(void *ptr) noexcept;

private:
	NPC(const std::string &name, const std::string &type, int x, int y, int health = 100);

	static inline Pool<NPC> pool{100, true};
	std::string _name; // This should be a fixed-length string (char[N]) if heap fragmentation is a concern
	std::string _type; // This should be an enum class if heap fragmentation is a concern
	int _x, _y, _health;
};

