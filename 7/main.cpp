#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <stop_token>
#include <syncstream>
#include <thread>
#include <unordered_set>
#include <utility>

struct order {
	enum class region_code { West, Central, East, North, South };
	static constexpr char const *region_name(order::region_code r) {
		switch (r) {
		case region_code::West: return "West";
		case region_code::Central: return "Central";
		case region_code::East: return "East";
		case region_code::North: return "North";
		case region_code::South: return "South";
		default: std::unreachable();
		}
	};
	int id;
	// Instructions unclear, used floating point storage for currency,
	// now I have NaN in my bank account and the IRS is assembling a
	// trebuchet in front of my domicile
	//
	// This is the amount *in cents* for above reasons
	unsigned amount_cents;
	region_code region;
};

// This is stupid. This is just global state with extra steps. Hiding it
// inside static members of a class actually reduces readability.
// Robert C. Martin has made a career off of preaching mediocre advice
// as religious dogma and now I have to do this nonsense because it's in
// the rubric >:(
struct OrderPipeline {
	static inline std::condition_variable raw_ready{}, ok_ready{};
	static inline std::mutex raw_mtx{}, ok_mtx{}, ids_mtx{};
	static inline std::queue<order> raw_orders{}, ok_orders{};
	static inline std::unordered_set<decltype(order::id)> ids{};
	static inline std::atomic<std::size_t> order_cnt{0};
	static inline std::atomic<std::size_t> ok_cnt{0};
};

// Regarding exceptions and the lack of a try/catch in this function:
// - random_device::random_device() can throw "on failure", but such
//   cases are not enumerated. In practice, implementations tend to just
//   call the random_device(const string&) with a known valid default
//   value. This is never going to throw.
// - lock_guard::lock_guard() can throw if the implementation is
//   equipped to detect and prevent a deadlock. Since no thread ever
//   holds more than one mutex at a time, it is not possible to deadlock
//   and thus this will never throw.
// - queue<T>::emplace() can throw if the T constructor throws (which
//   it doesn't) or if "an exception is thrown when inserting a single
//   element at either end", which I can only assume refers to memory
//   allocation failure, in which case there is no meaningful way to
//   recover and the program *should* terminate.
// Everything else is noexcept, or based on the documentation reasonably
// ought to never throw, and thus should terminate if it does.
void producer_fn(int job_id, std::ostream &log) noexcept {
	std::random_device dev{};
	std::default_random_engine rng{dev()};
	std::uniform_int_distribution<decltype(order::id)> rand_id{0};
	std::uniform_int_distribution<decltype(order::amount_cents)> rand_amount{100 * 10, 100 * 500};
	std::uniform_int_distribution<int> rand_region{0, 4};
	for (auto i = 0u; i < 200; ++i) {
		const auto id = rand_id(rng);
		{
			std::lock_guard guard{OrderPipeline::raw_mtx};
			OrderPipeline::raw_orders.emplace(
				id,
				rand_amount(rng),
				static_cast<order::region_code>(rand_region(rng))
			);
		}
		std::osyncstream{log} << std::chrono::system_clock::now() << ' ' << std::format(
			"INFO Producer thread {} generated order {}\n",
			job_id,
			id
		);
		OrderPipeline::order_cnt.fetch_add(1, std::memory_order_relaxed);
		OrderPipeline::raw_ready.notify_one();
	}
}

// Regarding exceptions and the lack of a try/catch in this function:
// - lock_guard::lock_guard() and unique_lock::unique_lock() can throw
//   if the implementation is equipped to detect and prevent a deadlock.
//   Since no thread ever holds more than one mutex at a time, it is not
//   possible to deadlock and thus these will never throw.
// - condition_variable::wait() only throws if the predicate does, which
//   it won't.
// - unordered_map::insert() can throw if "an exception is thrown for
//   any reason" (lol???), in which case there is no meaningful way to
//   recover and the program *should* terminate.
// Everything else is noexcept, or based on the documentation reasonably
// ought to never throw, and thus should terminate if it does.
void filter_fn(std::stop_token stop, int job_id, std::ostream &log) noexcept {
	std::stop_callback c{stop, [&]() noexcept {
		OrderPipeline::raw_ready.notify_all();
	}};
	while (true) {
		order o;
		{
			std::unique_lock guard{OrderPipeline::raw_mtx};
			OrderPipeline::raw_ready.wait(guard, [&]() noexcept {
				return !OrderPipeline::raw_orders.empty()
					|| stop.stop_requested();
			});
			if (stop.stop_requested() && OrderPipeline::raw_orders.empty()) break;
			o = OrderPipeline::raw_orders.front();
			OrderPipeline::raw_orders.pop();
		}
		// "Generate positive IDs in Stage 1" ok so this check for
		// positive IDs is redundant then
		if (o.id < 1) [[unlikely]] {
			std::osyncstream{log} << std::chrono::system_clock::now() << ' ' << std::format(
				"WARN Filter thread {} rejected order {} (non-positive order ID)\n",
				job_id,
				o.id
			);
			continue;
		}
		if (o.amount_cents < 100 * 150) {
			std::osyncstream{log} << std::chrono::system_clock::now() << ' ' << std::format(
				"WARN Filter thread {} rejected order {} (below minimum amount)\n",
				job_id,
				o.id
			);
			continue;
		}
		{
			std::lock_guard guard{OrderPipeline::ids_mtx};
			if (OrderPipeline::ids.contains(o.id)) {
				std::osyncstream{log} << std::chrono::system_clock::now() << ' ' << std::format(
					"WARN Filter thread {} rejected order {} (duplicate order ID)\n",
					job_id,
					o.id
				);
				continue;
			}
			OrderPipeline::ids.insert(o.id);
		}
		{
			std::lock_guard guard{OrderPipeline::ok_mtx};
			OrderPipeline::ok_orders.push(o);
		}
		std::osyncstream{log} << std::chrono::system_clock::now() << ' ' << std::format(
			"INFO Filter thread {} validated order {}\n",
			job_id,
			o.id
		);
		OrderPipeline::ok_cnt.fetch_add(1, std::memory_order_relaxed);
		OrderPipeline::ok_ready.notify_all();
	}
}

// Regarding exception handling in this function:
// See comment blocks preceding the above two functions; I/O exceptions
// are properly handled, everything else only throws on OOM and
// rightfully ought to terminate the program
void routing_fn(std::stop_token stop, order::region_code region, std::ostream &log) noexcept {
	std::stop_callback c{stop, [&]() noexcept {
		OrderPipeline::ok_ready.notify_all();
	}};
	const auto ri = static_cast<std::size_t>(region);
	const auto filename = std::format("region_{}.txt", ri);

	// The handout talks about these being atomic, but this worker
	// thread is perfectly capable of counting and printing them
	// itself, so that is what I am doing
	size_t order_cnt = 0;
	unsigned long total_revenue = 0;

	std::string buf{};
	std::ofstream f;
	f.exceptions(std::ios::failbit | std::ios::badbit);
	while (true) {
		order o;
		{
			std::unique_lock guard{OrderPipeline::ok_mtx};
			OrderPipeline::ok_ready.wait(guard, [&]() noexcept {
				return (!OrderPipeline::ok_orders.empty() && OrderPipeline::ok_orders.front().region == region)
					|| stop.stop_requested();
			});
			if (stop.stop_requested()) {
				if (OrderPipeline::ok_orders.empty()) break;
				if (OrderPipeline::ok_orders.front().region != region) {
					// Two threads can get in a loop of notify_one()
					// waking each other when neither region
					// matches, hence the use of notify_all().
					// Ask me how I know!
					OrderPipeline::ok_ready.notify_all();
					// No more orders can possibly match this region so
					// don't bother checking again
					if (OrderPipeline::ok_orders.size() == 1) break;
					continue;
				}
			}
			o = OrderPipeline::ok_orders.front();
			OrderPipeline::ok_orders.pop();
		}
		++order_cnt;
		total_revenue += o.amount_cents;
		buf.append(std::format("{},${}.{:02d}\n", o.id, o.amount_cents / 100, o.amount_cents % 100));
		try {
			if (!f.is_open()) f.open(filename);
			f << buf;
			buf.clear();
		} catch (const std::exception &e) {
			const auto msg = std::format(
				"ERROR Routing thread {} ({}) failed to write to {}: {}\n",
				ri,
				order::region_name(region),
				filename,
				e.what()
			);
			std::cerr << msg;
			std::osyncstream{log} << std::chrono::system_clock::now() << ' ' << msg;
		}
		std::osyncstream{log} << std::chrono::system_clock::now() << ' ' << std::format(
			"INFO Routing thread {} ({}) routed order {}\n",
			ri,
			order::region_name(region),
			o.id
		);
	}
	std::cout << std::format(
		"Region {} ({}): {} orders, Total Revenue: ${}.{:02d}\n",
		ri, order::region_name(region),
		order_cnt,
		total_revenue / 100, total_revenue % 100
	);
}

int main() {
	// Intentionally not enabling exceptions for the log file
	std::ofstream log{"pipeline.log"};
	if (log.fail() && !log.eof()) std::cerr << std::format("Failed to open pipeline.log for writing, log will be unavailable\n");
	// GITHUB UPDATE YOUR CODESPACES TOOLING SO I CAN USE <print> ALREADY!!!
	// C++ stream operators are disgusting and I'd use C-style I/O if I
	// thought I could get away with it 😾
	std::cout << "Order Processing Pipeline Results\n==================================\n";
	{
		// Destructors (and thus request_stop()) are called in reverse
		std::jthread r5{routing_fn, order::region_code::South, std::ref(log)},
			r4{routing_fn, order::region_code::North, std::ref(log)},
			r3{routing_fn, order::region_code::East, std::ref(log)},
			r2{routing_fn, order::region_code::Central, std::ref(log)},
			r1{routing_fn, order::region_code::West, std::ref(log)};
		std::jthread f1{filter_fn, 1, std::ref(log)},
			f2{filter_fn, 2, std::ref(log)},
			f3{filter_fn, 3, std::ref(log)},
			f4{filter_fn, 4, std::ref(log)};
		std::jthread p1{producer_fn, 1, std::ref(log)},
			p2{producer_fn, 2, std::ref(log)},
			p3{producer_fn, 3, std::ref(log)},
			p4{producer_fn, 4, std::ref(log)},
			p5{producer_fn, 5, std::ref(log)};
	}
	std::size_t order_cnt = OrderPipeline::order_cnt.load(std::memory_order_relaxed);
	std::size_t ok_cnt = OrderPipeline::ok_cnt.load(std::memory_order_relaxed);
	std::cout << std::format(
		"\n"
		"Total Valid Orders: {}\n"
		"Total Invalid Orders: {}\n"
		"Pipeline processing completed successfully.\n",
		ok_cnt, order_cnt - ok_cnt
	);
}
