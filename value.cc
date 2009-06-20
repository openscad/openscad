/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include "openscad.h"
#include <math.h>

Value& Value::operator = (const Value &v) {
	x = v.x;
	y = v.y;
	z = v.z;
	is_vector = v.is_vector;
	is_nan = v.is_nan;
	return *this;
}

Value Value::operator + (const Value &v) const {
	if (is_nan || v.is_nan)
		return Value();
	if (is_vector && v.is_vector)
		return Value(x + v.x, y + v.y, z + v.z);
	if (!is_vector && !v.is_vector)
		return Value(x + v.x);
	return Value();
}

Value Value::operator - (const Value &v) const {
	if (is_nan || v.is_nan)
		return Value();
	if (is_vector && v.is_vector)
		return Value(x - v.x, y - v.y, z - v.z);
	if (!is_vector && !v.is_vector)
		return Value(x - v.x);
	return Value();
}

Value Value::operator * (const Value &v) const {
	if (is_nan || v.is_nan)
		return Value();
	if (is_vector && v.is_vector) {
		double nx = (y-v.y)*(z-v.z) - (z-v.z)*(y-v.y);
		double ny = (z-v.z)*(x-v.x) - (x-v.x)*(z-v.z);
		double nz = (x-v.x)*(y-v.y) - (y-v.y)*(x-v.x);
		return Value(nx, ny, nz);
	}
	if (is_vector) {
		return Value(x * v.x, y * v.x, z * v.x);
	}
	if (v.is_vector) {
		return Value(x * v.x, x * v.y, x * v.z);
	}
	return Value(x * v.x);
}

Value Value::operator / (const Value &v) const {
	if (is_nan || v.is_nan || is_vector || v.is_vector)
		return Value();
	return Value(x / v.x);
}

Value Value::operator % (const Value &v) const {
	if (is_nan || v.is_nan || is_vector || v.is_vector)
		return Value();
	return Value(fmod(x, v.x));
}

Value Value::inv() const {
	if (is_nan)
		return Value();
	if (is_vector)
		return Value(-x, -y, -z);
	return Value(-x);
}

