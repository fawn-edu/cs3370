// main.cpp
// Fawn Sannar <10725695@uvu.edu>

#include "frustum.hpp"
#include <cstdio> // I would rather die than use cursed c++ streams

int main() {
	int c;
	double w1, l1, w2, l2, h;
	char line[128] = {}; // 128 should be enough digits for one double

	printf("Top rectangle width     : ");
	fgets(line, sizeof line, stdin);
	if ((c = sscanf(line, "%lf", &w1)) != 1) {
		puts("Invalid value"); return 2;
	} else if (w1 <= 0.0) { puts("Values must be positive"); return 2; }

	printf("Top rectangle length    : ");
	fgets(line, sizeof line, stdin);
	if ((c = sscanf(line, "%lf", &l1)) != 1) {
		puts("Invalid value"); return 2;
	} else if (l1 <= 0.0) { puts("Values must be positive"); return 2; }

	printf("Bottom rectangle width  : ");
	fgets(line, sizeof line, stdin);
	if ((c = sscanf(line, "%lf", &w2)) != 1) {
		puts("Invalid value"); return 2;
	} else if (w2 <= 0.0) { puts("Values must be positive"); return 2; }

	printf("Bottom rectangle length : ");
	fgets(line, sizeof line, stdin);
	if ((c = sscanf(line, "%lf", &l2)) != 1) {
		puts("Invalid value"); return 2;
	} else if (l2 <= 0.0) { puts("Values must be positive"); return 2; }

	printf("Frustum height          : ");
	fgets(line, sizeof line, stdin);
	if ((c = sscanf(line, "%lf", &h)) != 1) {
		puts("Invalid value"); return 2;
	} else if (h <= 0.0) { puts("Values must be positive"); return 2; }

	const auto f = RectangularFrustum(w1, l1, w2, l2, h);
	printf("Volume: %lf\nSurface area: %lf\n", f.volume(), f.surfaceArea());
	return 0;
}
