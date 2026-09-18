#include <iostream>
#include <vector>
#include "assignment2_test_harness.hpp"
#include "NPC.hpp"

int main() {
	// Create several NPCs
	std::vector<NPC*> npcs;

	// Spawn a wave of enemies
	for (int i = 0; i < 10; ++i) {
		npcs.push_back(NPC::spawn(
			"Goblin_" + std::to_string(i),
			"enemy",
			i * 10, i * 10, // Position
			50 + i * 5 // Health
		));
	}

	// Show one NPC
	std::cout << *npcs[5] << std::endl;

	// Delete some NPCs (simulating death)
	delete npcs[3];
	delete npcs[5];
	npcs[3] = nullptr;
	npcs[5] = nullptr;

	// Profile the pool
	NPC::profile();

	// Spawn replacements
	npcs[3] = NPC::spawn("Dragon_Boss", "boss", 50, 50, 500);
	npcs[5] = NPC::spawn("Shopkeeper", "merchant", 25, 25, 100);

	// Clean up all NPCs
	for (auto* npc : npcs) {
		delete npc;
	}

	// Final profile
	NPC::profile();

	std::cout << "=====================================================\n"
			  << " CS 3370 Assignment 2 -- NPC Memory Pool self-test\n"
			  << "=====================================================\n";

	test_size_and_alignment();
	test_output_operator();
	test_pool_geometry();
	test_exhaustion();
	test_slot_reuse();
	test_interleaved_and_integrity();
	test_full_cycle();
	test_churn();
	test_profile();

	std::cout << "\n=====================================================\n"
			  << " SUMMARY: " << g_pass << " passed, "
			  << g_fail << " failed, " << g_warn << " warnings\n";
	if (g_fail == 0)
		std::cout << " All required checks passed. Re-read your Pool "
					 "destructor\n for leaks, then submit.\n";
	else
		std::cout << " Fix the [FAIL] items above before submitting.\n";
	std::cout << "=====================================================\n";

	return g_fail == 0 ? 0 : 1;
}
