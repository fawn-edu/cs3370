// NPC.cpp
// Fawn Sannar <10725695@uvu.edu>

#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include "NPC.hpp"

using std::string;

NPC::NPC(const string &name, const string &type, int x, int y, int health)
	: _name{name}
	, _type{type}
	, _x{x}
	, _y{y}
	, _health{health} {}

NPC *NPC::spawn(const string &name, const string &type, int x, int y, int health) {
	return new NPC{name, type, x, y, health};
}

void NPC::profile() { pool.profile(); }

std::ostream &operator<<(std::ostream &s, const NPC &npc) {
	return s << std::format(
		"NPC[name='{}', type='{}', pos=({},{}), hp={}]",
		npc._name,
		npc._type,
		npc._x,
		npc._y,
		npc._health
	);
}

void *NPC::operator new(std::size_t) { return pool.allocate(); }

void NPC::operator delete(void *ptr) noexcept {
	pool.deallocate(static_cast<NPC*>(ptr));
}
