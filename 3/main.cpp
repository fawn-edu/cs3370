#include "test_bigint.h"
#include "perf_bigint.h"
// #include <print>

static_assert(NumberLike<int>);
static_assert(NumberLike<double>);
static_assert(NumberLike<BigInt>);
static_assert(!NumberLike<std::string>);

int main() {
	runPerformanceReport();
	// For report item 3
	// {
	// 	constexpr auto n = 10000;
	// 	const auto time_waster_9000 = factorial<BigInt>(n);
	// 	std::print(
	// 		"factorial<BigInt>({}).digitCount() = {}\n"
	// 		"factorial<BigInt>({}).capacity()   = {}\n",
	// 		n, time_waster_9000.digitCount(),
	// 		n, time_waster_9000.capacity()
	// 	);
	// }
	return runRequiredChecks() ? 1 : 0;
}
