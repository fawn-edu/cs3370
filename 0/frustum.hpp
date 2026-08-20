// frustum.hpp
// Fawn Sannar <10725695@uvu.edu>

#include <cassert>
#include <cmath>
#include "rectangle.hpp"

class RectangularFrustum {
public:
	RectangularFrustum(double w1, double l1, double w2, double l2, double height) : _rTop{w1, l1}, _rBottom{w2, l2}, _height{height} {
		// "Disallow non-positive values"
		// C++26 contracts would be good for this
		assert(_rTop.Width() > 0.0 && _rTop.Height() > 0.0 && _rBottom.Width() > 0.0 && _rBottom.Height() > 0.0 && _height > 0.0);
	}

	double volume() const {
		const auto a1 = _rTop.Width() * _rTop.Height();
		const auto a2 = _rBottom.Width() * _rBottom.Height();
		return _height * (a1 + a2 + sqrt(a1 * a2)) / 3.0;
	}

	double surfaceArea() const {
		const auto a1 = _rTop.Width() * _rTop.Height();
		const auto a2 = _rBottom.Width() * _rBottom.Height();
		// Factored out _height
		return a1 + a2 + _height * (_rTop.Width() + _rBottom.Width() + _rTop.Height() + _rBottom.Height());
	}
private:
	Rectangle _rTop, _rBottom;
	double _height;
};
