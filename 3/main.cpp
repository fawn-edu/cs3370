#include "test_bigint.hpp"
#include "perf_bigint.hpp"

static_assert(NumberLike<int>);
static_assert(NumberLike<double>);
static_assert(NumberLike<BigInt>);
static_assert(!NumberLike<std::string>);

int main() {
	runPerformanceReport();
	return runRequiredChecks() ? 1 : 0;
}
