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

#define _USE_MATH_DEFINES
#include <cmath>

#include "calc.h"
#include "grid.h"

/*!
	Returns the number of subdivision of a whole circle, given radius and
	the three special variables $fn, $fs and $fa
*/
int Calc::get_fragments_from_r(double r, double fn, double fs, double fa)
{
	// FIXME: It would be better to refuse to create an object. Let's do more strict error handling
	// in future versions of OpenSCAD
	if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return 3;
	if (fn > 0.0) return static_cast<int>(fn >= 3 ? fn : 3);
	return static_cast<int>(ceil(fmax(fmin(360.0 / fa, r*2*M_PI / fs), 5)));
}

/*!
	Returns the number of slices for a linear_extrude with twist.
	Given height, twist, and the three special variables $fn, $fs and $fa
*/
int Calc::get_slices_from_height_twist(double h, double twist, double fn, double fs, double fa)
{
	double abs_twist = fabs(twist);
	// 180 twist per slice is worst case, guaranteed non-manifold.
	// Make sure we have at least 3 slices per 360 twist
	double min_slices = fmax(ceil(abs_twist / 120.0), 1.0);
	if (h < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return min_slices;
	if (fn > 0.0) return static_cast<int>(fmax(ceil(abs_twist / 360.0 * fn), min_slices));
	return static_cast<int>(fmax(ceil(fmin(abs_twist / fa, h / fs)), min_slices));
}
