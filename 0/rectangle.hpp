// rectangle.h
// Fawn Sannar <10725695@uvu.edu>

#pragma once

class Rectangle {
public:
	Rectangle(double w, double h) : _w{w}, _h{h} {}
	void setWidth(double w) { _w = w; }
	void setHeight(double h) { _h = h; }
	double Width() const { return _w; }
	double Height() const { return _h; }
private:
	// I am making these private with getters & setters because the
	// assignment explicitly calls for this. However, the getters and
	// setters are pure, so there is absolutely no reason to hide them
	// behind functions. This is OOP brainrot. Just make these members
	// public and read/write to them directly.
	double _w, _h;
};

// What this class should have been:
/* struct Rectangle { double w, h; }; */
