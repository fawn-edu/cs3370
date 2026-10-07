#include "bigint.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ranges>
#include <stdexcept>
#include <ostream>

using std::size_t, std::uint64_t, std::uint32_t;

BigInt::BigInt() : digits{0} {}

BigInt::BigInt(long long n) : negative(n < 0) {
	unsigned long long m = n;
	if (negative) m = -m;
	for (; m; m /= 10) {
		digits.emplace_back(m % 10);
	}
	trim();
}

BigInt::BigInt(const std::string& s) {
	if (s.empty()) throw std::invalid_argument{"s"};
	BigInt inc{1}, res{0};
	for (const char *c = &s.back(); c >= s.data(); --c) {
		if (c == s.data() && *c == '-') {
			if (s.size() == 1) throw std::invalid_argument{"s"};
			res.negative = true;
			break;
		}
		if ('0' > *c || *c > '9') throw std::invalid_argument{"s"};
		res += inc * (*c - '0');
		inc *= 10;
	}
	*this = std::move(res);
	trim();
}

BigInt::operator bool() const {
	return digitCount() != 1 || digits.back() != 0;
	// return std::ranges::any_of(
	// 	std::ranges::reverse_view{digits},
	// 	[](const auto& digit) { return digit != 0; }
	// );
}

BigInt& BigInt::operator+=(const BigInt& rhs) {
	if (negative != rhs.negative) {
		// Flipping lhs sign is cheaper than allocating a temporary
		// rhs with negation operator
		negative = !negative;
		if (digitCount() > rhs.digitCount()) {
			// -A + +b => -(+A - +b) => -(A - b)
			// +A + -b => -(-A - -b) => -(A - b)
			*this -= rhs;
			negative = !negative;
		} else {
			// -a + +B => +B - +a => B - a
			// +a + -B => -B - -a => B - a
			*this = rhs - *this;
		}
		return *this;
	}

	auto carry = 0;
	for (auto i = 0u; i < digitCount() || i < rhs.digitCount(); ++i) {
		auto d = at(i) + rhs.at(i) + carry;
		carry = d / 10;
		const auto digit = static_cast<unsigned char>(d % 10);
		if (i < digitCount()) digits[i] = digit; else digits.emplace_back(digit);
	}
	if (carry) digits.emplace_back(carry);
	trim();
	return *this;
}

BigInt& BigInt::operator-=(const BigInt& rhs) {
	if (negative != rhs.negative) {
		// Flipping lhs sign is cheaper than allocating a temporary
		// rhs with negation operator
		negative = !negative;
		if (digitCount() > rhs.digitCount()) {
			// -A - +b => -(+A + +b) => -(A + b)
			// +A - -b => -(-A + -b) => -(A + b)
			*this -= rhs;
			negative = !negative;
		} else {
			// -a - +B => +B + +a => B + a
			// +a - -B => -B + -a => B + a
			*this = rhs - *this;
		}
		return *this;
	}

	// Always subtract the larger magnitude from the lesser
	if (digitCount() < rhs.digitCount()) {
		// a - B = -(B - a)
		*this = rhs - *this;
		negative = !negative;
		return *this;
	}

	bool borrow = false;
	for (auto i = 0u; i < digitCount() || i < rhs.digitCount(); ++i) {
		auto d = at(i) - rhs.at(i) - borrow;
		borrow = d < 0;
		if (borrow) d += 10;
		assert(0 <= d && d < 10);
		const auto digit = static_cast<unsigned char>(d);
		if (i < digitCount()) digits[i] = digit; else digits.emplace_back(digit);
	}
	assert(!borrow);
	trim();
	return *this;
}

BigInt& BigInt::operator*=(const BigInt& rhs) { return *this = mul(*this, rhs); }

BigInt BigInt::operator-() const {
	BigInt n{*this};
	n.negative = !negative;
	return n;
}

BigInt& BigInt::operator++() { return *this += 1; }

BigInt BigInt::operator++(int) {
	const BigInt old{*this};
	++*this;
	return old;
}

BigInt& BigInt::operator--() { return *this -= 1; }

BigInt BigInt::operator--(int) {
	const BigInt old{*this};
	--*this;
	return old;
}

const unsigned char& BigInt::operator[](size_t i) const { return digits.at(i); }
unsigned char& BigInt::operator[](size_t i) { return digits.at(i); }

size_t BigInt::digitCount() const { return digits.size(); }

bool BigInt::isNegative() const { return *this && negative; }

std::string BigInt::toString() const { return std::format("{}", *this); }

bool operator==(const BigInt& a, const BigInt& b) {
	if (a.digitCount() != b.digitCount()) return false;
	return std::equal(a.digits.begin(), a.digits.end(), b.digits.begin());
}

std::strong_ordering operator<=>(const BigInt& a, const BigInt& b) {
	const auto sign = !a.isNegative() <=> !b.isNegative();
	if (sign != std::strong_ordering::equal) return sign;

	const auto len = a.digitCount() <=> b.digitCount();
	if (len != std::strong_ordering::equal) return len;

	for (auto i = a.digitCount() - 1; i; --i) {
		const auto digits = a.digits[i] <=> b.digits[i];
		if (digits != std::strong_ordering::equal) return digits;
	}
	return std::strong_ordering::equal;
}

void BigInt::trim() {
	while (!digits.empty() && digits.back() == 0) digits.pop_back();
	if (digits.empty()) {
		digits.emplace_back(0);
		negative = false;
	}
}

unsigned char BigInt::at(size_t i) const { return i < digitCount() ? digits[i] : 0; }

size_t BigInt::capacity() const { return digits.capacity(); }

BigInt BigInt::mul(const BigInt& lhs, const BigInt& rhs) {
	constexpr auto split = [](const BigInt& n, auto idx) -> std::pair<BigInt, BigInt> {
		if (idx >= n.digitCount()) return {0, n};
		std::pair<BigInt, BigInt> halves{};
		halves.second.digits = decltype(n.digits){n.digits.begin(), n.digits.begin() + idx};
		halves.first.digits = decltype(n.digits){n.digits.begin() + idx, n.digits.end()};
		return halves;
	};

	// Stupid hack because comparison and two-way conversion is cheaper
	// than allocating a million single-digit BigInts for leaf recursion
	// calls. If both operands can each fit in a uint32_t, the product
	// will fit into a uint64_t.
	// Without this hack, factorial<BigInt>(10000) on -O2 takes 10 whole
	// minutes on my PC, which is absolutely humiliating performance.
	// With this hack, it instead takes just under 1 minute, which is
	// still abysmal but at least it's single digits. FML
	static const BigInt u32_max = std::numeric_limits<uint32_t>::max();
	constexpr auto as_u64 = [](const BigInt& n) -> uint64_t {
		uint32_t result = 0, inc = 1;
		for (const auto& digit : n.digits) {
			result += inc * digit;
			inc *= 10;
		}
		return result;
	};
	constexpr auto from_u64 = [](std::uint64_t n) {
		BigInt v{};
		v.digits.clear();
		for (; n; n /= 10) v.digits.emplace_back(n % 10);
		return v;
	};

	if (!lhs || !rhs) return 0;
	// HACK: If the product can fit in a uint64_t, just do that directly
	if (lhs < u32_max && rhs < u32_max) return from_u64(as_u64(lhs) * as_u64(rhs));
	
	const auto half = std::max(lhs.digitCount(), rhs.digitCount()) / 2;
	auto [a, b] = split(lhs, half);
	auto [c, d] = split(rhs, half);
	BigInt ac = mul(a, c);
	const BigInt bd = mul(b, d);
	// Reuse existing BigInts because this function is recursive and the
	// allocations are out of control
	a += b; c += d;
	BigInt ad_bc = mul(a, c) - ac - bd;
	ac.digits.insert(ac.digits.begin(), size_t{2 * half}, 0);
	ad_bc.digits.insert(ad_bc.digits.begin(), size_t{half}, 0);
	ad_bc += ac + bd;
	ad_bc.negative = lhs.negative != rhs.negative;
	return ad_bc;
}

BigInt operator+(BigInt lhs, const BigInt& rhs) { return lhs += rhs; }

BigInt operator-(BigInt lhs, const BigInt& rhs) { return lhs -= rhs; }

BigInt operator*(BigInt lhs, const BigInt& rhs) { return lhs *= rhs; }

std::ostream& operator<<(std::ostream& os, const BigInt& b) { return os << b.toString(); }
