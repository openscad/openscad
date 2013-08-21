#include "mathc99.h"

#ifdef WIN32
#include <algorithm>

double trunc(double a) {
	return (a >= 0) ? floor(a) : ceil(a);
}

double round(double a) {
	return a < 0 ? ceil(a - 0.5f) : floor(a + 0.5f);
}

float fmin(float a, float b) {
	return std::min(a,b);
}

float fmax(float a, float b) {
	return std::max(a,b);
}

#endif
