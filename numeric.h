#pragma once

#include <concepts>

template <typename T>
concept NumberLike = std::copyable<T> && std::constructible_from<int> && requires(T a, T b) {
	{ a + b } -> std::convertible_to<T>;
	{ a * b } -> std::convertible_to<T>;
};

template <NumberLike T>
T power(T base, unsigned exponent) {
	if (!exponent) return T{1};
	T n = power(base * base, exponent / 2);
	if (exponent & 1) n *= base;
	return n;
}

template <NumberLike T>
T factorial(unsigned n) {
	if (n < 2) return T{n};
	T f{1};
	for (auto i = 2u; i <= n; ++i) f *= i;
	return f;
}

template <NumberLike T>
T fibonacci(unsigned n) {
	T a{0}, b{1};
	while (n--) {
		std::swap(a, b);
		b += a;
	}
	return a;
}
