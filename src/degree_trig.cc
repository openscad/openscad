/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
//
// Trigonometry function taking degrees, accurate for 30, 45, 60 and 90, etc.
//
#include "degree_trig.h"
#include "math.h"
#include <limits>

#define M_SQRT3   1.73205080756887719318 /* sqrt(3)   */
#define M_SQRT3_4 0.86602540378443859659 /* sqrt(3/4) == sqrt(3)/2 */
#define M_SQRT1_3 0.57735026918962573106 /* sqrt(1/3) == sqrt(3)/3 */

static inline double deg2rad(double x)
{
	return x * M_PI / 180.0;
}

// this limit assumes 26+26=52 bits mantissa
// comment/undefine it to disable domain check
#define TRIG_HUGE_VAL ((1L<<26)*360.0*(1L<<26))

double sin_degrees(double x)
{
	// use positive tests because of possible Inf/NaN
	if (x < 360.0 && x >= 0.0) {
		// Ok for now
	} else
#ifdef TRIG_HUGE_VAL
	if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
	{
		double revolutions = floor(x/360.0);
		x -= 360.0*revolutions;
	}
#ifdef TRIG_HUGE_VAL
	else {
		// total loss of computational accuracy
		// the result would be meaningless
		return std::numeric_limits<double>::quiet_NaN();
	}
#endif
	bool oppose = x >= 180.0;
	if (oppose) x -= 180.0;
	if (x > 90.0) x = 180.0 - x;
	if (x < 45.0) {
		if (x == 30.0) x = 0.5;
		else x = sin(deg2rad(x));
	} else if (x == 45.0) {
		x = M_SQRT1_2;
	} else if (x == 60.0) {
		x = M_SQRT3_4;
	} else { // Inf/Nan would fall here
		x = cos(deg2rad(90.0-x));
	}
	return oppose ? -x : x;
}

double cos_degrees(double x)
{
	// use positive tests because of possible Inf/NaN
	if (x < 360.0 && x >= 0.0) {
		// Ok for now
	} else
#ifdef TRIG_HUGE_VAL
	if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
	{
		double revolutions = floor(x/360.0);
		x -= 360.0*revolutions;
	}
#ifdef TRIG_HUGE_VAL
	else {
		// total loss of computational accuracy
		// the result would be meaningless
		return std::numeric_limits<double>::quiet_NaN();
	}
#endif
	bool oppose = x >= 180.0;
	if (oppose) x -= 180.0;
	if (x > 90.0) {
		x = 180.0 - x;
		oppose = !oppose;
	}
	if (x > 45.0) {
		if (x == 60.0) x = 0.5;
		else x = sin(deg2rad(90.0-x));
	} else if (x == 45.0) {
		x = M_SQRT1_2;
	} else if (x == 30.0) {
		x = M_SQRT3_4;
	} else { // Inf/Nan would fall here
		x = cos(deg2rad(x));
	}
	return oppose ? -x : x;
}

double tan_degrees(double x)
{
	int cycles = floor((x) / 180.0);
	// use positive tests because of possible Inf/NaN
	if (x < 180.0 && x >= 0.0) {
		// Ok for now
	} else
#ifdef TRIG_HUGE_VAL
	if (x < TRIG_HUGE_VAL && x > -TRIG_HUGE_VAL)
#endif
	{
		x -= 180.0*cycles;
	}
#ifdef TRIG_HUGE_VAL
	else {
		// total loss of computational accuracy
		// the result would be meaningless
		return std::numeric_limits<double>::quiet_NaN();
	}
#endif
	bool oppose = x > 90.0;
	if (oppose) x = 180.0-x;
	if (x == 0.0) {
		x = (cycles % 2) == 0 ? 0.0 : -0.0;
	} else if (x == 30.0) {
		x = M_SQRT1_3;
	} else if (x == 45.0) {
		x = 1.0;
	} else if (x == 60.0) {
		x = M_SQRT3;
	} else if (x == 90.0) {
		x = (cycles % 2) == 0 ?
			std::numeric_limits<double>::infinity() :
			-std::numeric_limits<double>::infinity();
	} else {
		x = tan(deg2rad(x));
	}
	return oppose ? -x : x;
}

