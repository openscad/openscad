#include "mathc99.h"

#ifdef WIN32
#include <algorithm>

double round(double a) {
	return a > 0 ? floor(a+0.5) : ceil(a-0.5);
}

float fmin(float a, float b) {
	return std::min(a,b);
}

float fmax(float a, float b) {
	return std::max(a,b);
}

#endif
