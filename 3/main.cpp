#include "numeric.h"
#include "test_bigint.h"
#include "perf_bigint.h"
#include <cassert>
#include <print>

static_assert(NumberLike<int>);
static_assert(NumberLike<double>);
static_assert(NumberLike<BigInt>);
static_assert(!NumberLike<std::string>);

int main() {
	// std::println("5 == -5 = {} (expected {})", BigInt{5} == BigInt{-5}, 5 == -5);
	// std::println("12 < 13 = {} (expected {})", BigInt{12} < BigInt{13}, 12 < 13);
	// std::println("-100 < -5 = {} (expected {})", BigInt{-100} < BigInt{-5}, -100 < -5);
	// std::println("3 - 5 = {} (expected {})", BigInt{3} - BigInt{5}, 3 - 5);
	// std::println("5 - -3 = {} (expected {})", BigInt{5} - BigInt{-3}, 5 - -3);
	// std::println("-1 - 1 = {} (expected {})", BigInt{-1} - BigInt{1}, -1 - 1);
	// std::println("-3 * 4 = {} (expected {})", BigInt{-3} * BigInt{4}, -3 * 4);
	
	// According to the grader:
	//
	// "The harness’s check 6 doesn’t search for the answer. It already
	// knows it’s 4782 and only confirms that F(4781) has 999 digits and
	// F(4782) has 1,000... So main.cpp needed a loop that builds each
	// Fibonacci number from the previous two until one reaches 1,000
	// digits, then prints its index... This seemed to be something
	// missed by the greater majority of everyone, so I will be
	// contacting the professor about this"
	{
		// a = F(0) = 0, b = F(1) = 1, neither has 1000 digits
		BigInt a{0}, b{1};
		// Therefore, begin at F(2)
		auto i = 2u;
		for (;; ++i) {
			std::swap(a, b);
			b += a;
			if (b.digitCount() == 1000) break;
		}
		std::println("The first fibonacci number with 1000 digits is F({})", i);
		assert(i == 4782);
	}

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
