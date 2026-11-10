// This file was written by a clanker and I have made no modifications
// to it beyond this comment. I am morally opposed to the development
// and use of so-called "AI" (especially in an educational environment)
// and I am ONLY doing this because the assignment specifically requires
// me to.
#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <utility>

namespace stage_timer_detail {
	struct Stats {
		double milliseconds = 0.0;
		std::size_t calls = 0;
	};

	inline std::map<std::string, Stats> totals;
	inline std::mutex totals_mutex;
}

class StageTimer {
	using Clock = std::chrono::steady_clock;

	std::string name_;
	Clock::time_point start_ = Clock::now();

public:
	explicit StageTimer(std::string name)
		: name_(std::move(name)) {}

	~StageTimer() {
		const auto elapsed = std::chrono::duration<double, std::milli>(
			Clock::now() - start_).count();

		std::lock_guard lock(stage_timer_detail::totals_mutex);
		auto& stats = stage_timer_detail::totals[name_];
		stats.milliseconds += elapsed;
		++stats.calls;
	}

	StageTimer(const StageTimer&) = delete;
	StageTimer& operator=(const StageTimer&) = delete;
};

inline void print_stage_report() {
	std::lock_guard lock(stage_timer_detail::totals_mutex);

	std::cerr << "\nStage timing report:\n";
	for (const auto& [name, stats] : stage_timer_detail::totals) {
		std::cerr << name << ": "
			<< stats.milliseconds << " ms ("
			<< stats.calls << " calls)\n";
	}
}
