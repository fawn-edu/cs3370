#pragma once

#include <compare>
#include <concepts>
#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

class BigInt {
	// T = unsigned char makes some debug stuff easier, but in practice
	// it ought to be register-width such as std::size_t
	using T = unsigned char; // TODO: std::size_t
	static_assert(std::unsigned_integral<T>);
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
	BigInt& operator/=(const BigInt& rhs);
	BigInt& operator%=(const BigInt& rhs);

	// --- unary, increment, decrement ---------------------------------
	BigInt operator-() const;
	BigInt& operator++();
	BigInt operator++(int);
	BigInt& operator--();
	BigInt operator--(int);

	// --- inspection ---------------------------------------------------
	int operator[](std::size_t i) const; // i-th decimal digit from the right
	std::size_t digitCount() const; // 0 has one digit
	bool isNegative() const;
	std::string toString() const;

	// --- comparison ---------------------------------------------------
	friend bool operator==(const BigInt& a, const BigInt& b);
	friend std::strong_ordering operator<=>(const BigInt& a, const BigInt& b);

private:
	T sign_ext() const; // Sign extension limb
	void trim(); // Remove redundant sign extension limbs
	void division(const BigInt& rhs, BigInt& quotient, BigInt& remainder);
	std::vector<T> limbs;
};

// --- non-member binary operators, built on the compound ones ---------
BigInt operator+(BigInt lhs, const BigInt& rhs);
BigInt operator-(BigInt lhs, const BigInt& rhs);
BigInt operator*(BigInt lhs, const BigInt& rhs);
BigInt operator/(BigInt lhs, const BigInt& rhs);
BigInt operator%(BigInt lhs, const BigInt& rhs);
std::ostream& operator<<(std::ostream& os, const BigInt& b);
