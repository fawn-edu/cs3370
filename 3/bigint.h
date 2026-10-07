#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <format>
#include <iosfwd>
#include <string>
#include <vector>

class BigInt {
	friend struct std::formatter<BigInt>;
public:
	// --- construction and conversion ---------------------------------
	BigInt(); // the value 0
	BigInt(long long n); // implicit, on purpose
	explicit BigInt(const std::string& s); // "42", "-17", "007"
	explicit operator bool() const; // true if nonzero

	// --- compound assignment: the real arithmetic lives here ---------
	BigInt& operator+=(const BigInt& rhs);
	BigInt& operator-=(const BigInt& rhs);
	BigInt& operator*=(const BigInt& rhs);

	// --- unary, increment, decrement ---------------------------------
	BigInt operator-() const;
	BigInt& operator++();
	BigInt operator++(int);
	BigInt& operator--();
	BigInt operator--(int);

	// --- inspection ---------------------------------------------------
	const unsigned char& operator[](std::size_t i) const; // i-th decimal digit from the right
	unsigned char& operator[](std::size_t i); // i-th decimal digit from the right
	std::size_t digitCount() const; // 0 has one digit
	bool isNegative() const;
	std::string toString() const;

	// --- comparison ---------------------------------------------------
	friend bool operator==(const BigInt& a, const BigInt& b);
	friend std::strong_ordering operator<=>(const BigInt& a, const BigInt& b);

	std::size_t capacity() const; // Exposes digits.capacity() because the report wants it
private:
	static BigInt mul(const BigInt& lhs, const BigInt& rhs); // Karatsuba algorithm
	void trim(); // Removes leading zeroes
	unsigned char at(std::size_t i) const; // Like operator[], but returns 0 when out of bounds

	std::vector<unsigned char> digits;
	bool negative = false;
};

// --- non-member binary operators, built on the compound ones ---------
BigInt operator+(BigInt lhs, const BigInt& rhs);
BigInt operator-(BigInt lhs, const BigInt& rhs);
BigInt operator*(BigInt lhs, const BigInt& rhs);
std::ostream& operator<<(std::ostream& os, const BigInt& b);

template<>
struct std::formatter<BigInt> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const BigInt& b, std::format_context& ctx) const {
		std::string s{b.digits.begin(), b.digits.end()};
		for (auto& c : s) c += '0';
		if (b && b.negative) s += '-';
		std::ranges::reverse(s);
		return std::format_to(ctx.out(), "{}", s);
	}
};
