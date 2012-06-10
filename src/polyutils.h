#ifndef POLYUTILS_H_
#define POLYUTILS_H_

/* PolySet Utility Functions */
#include "function.h"
#include "expression.h"
#include "context.h"
#include "builtin.h"
#include <sstream>
#include <ctime>
#include "mathc99.h"
#include <algorithm>
#include "stl-utils.h"
#include "printutils.h"
#include <Eigen/Core>
#include "handle_dep.h"
#include <Magick++.h>
#include <string>

using namespace Magick;



PolySet * readPolySetFromImage( const Filename, bool, double, int );

PolySet * readPolySetFromRiseGroundBase( const Filename, bool, double, int );

PolySet * readPolySetFromSTL( const Filename , int );

PolySet * readPolySetFromDXF( const Filename , std::string , double , double , double , double , double , double , int );

#endif
