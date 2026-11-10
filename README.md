# Programming assignment 5: pulse data reduction

Fawn Sannar <10725695@uvu.edu> | `10725695`

The program can be compiled and ran using `gage2scope.ini` by invoking `make`.

## Reflection

I noticed that the moving average is a convolution, so I initially tried to be smart and write a helper that would let me perform that using iterators rather than a `for` loop. It works, but I ultimately decided it made the code *less* readable, so I removed it from `main.cpp`. However, I still think it's neat, and it would make it easy to swap window size and convolution kernel (weights), so I'll put it in this reflection:

```cpp
#include <ranges>
#include <valarray>

using namespace std;

// Get a window into src of width W centered on idx, with out-of-bounds
// indicies receiving a value of 0
// If W is even, the "center index" is rounded up. That's not relevant
// in this assignment though since W only ever needs to be 7 lol
template<size_t W> requires (W > 0)
valarray<int> window(span<int> src, ptrdiff_t idx) {
	constexpr ptrdiff_t HALF = W / 2;
	constexpr ptrdiff_t END = (W & 1) + HALF;
	valarray<int> dst(W);
	const auto begin = max(-HALF, -idx);
	const auto cnt = min(END, static_cast<ptrdiff_t>(src.size()) - idx) - begin;
	if (cnt < 1) return dst;
	copy_n(&src[idx + begin], min<size_t>(cnt, dst.size()), &dst[HALF + begin]);
	return dst;
}

// I'd constexpr this and put it inside the lambda below if valarray had
// a constexpr constructor
const valarray<int> weights{1, 2, 3, 3, 3, 2, 1};

// Then, in main(), replacing the for-loop that populates smoothed:
const auto weights_sum = weights.sum();
vector<int> smoothed{data};
ranges::transform(views::iota(0), smoothed.begin(), [&weights, &weights_sum](size_t i){
	return (window<7>(data, i) * weights).sum() / weights_sum;
});
```
