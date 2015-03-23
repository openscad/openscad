#pragma once

#if defined(_MSC_VER)

// only for native windows builds, not MXE

#include <cmath>
//for native win32 builds we need to provide C99 math functions by ourselves
double trunc(double a);
double round(double a);
float fmin(float a, float b);
float fmax(float a, float b);

#else

#define _USE_MATH_DEFINES
#include <math.h>

#endif
