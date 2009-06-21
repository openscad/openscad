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

Value::Value(const Value &v1, const Value &v2, const Value &v3)
{
	if (v1.is_nan || v1.is_vector)
		goto create_nan;
	if (v2.is_nan || v2.is_vector)
		goto create_nan;
	if (v3.is_nan || v3.is_vector)
		goto create_nan;

	x = v1.x;
	y = v1.y;
	z = v1.z;

	is_vector = true;
	is_range = false;
	is_string = false;
	is_nan = false;
	return;

create_nan:
	x = 0;
	y = 0;
	z = 0;

	is_vector = false;
	is_range = false;
	is_string = false;
	is_nan = true;
}

Value& Value::operator = (const Value &v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	is_vector = v.is_vector;
	is_nan = v.is_nan;
	return *this;
}

Value Value::operator + (const Value &v) const
{
	if (is_nan || v.is_nan)
		return Value();
	if (is_vector && v.is_vector)
		return Value(x + v.x, y + v.y, z + v.z);
	if (!is_vector && !v.is_vector)
		return Value(x + v.x);
	return Value();
}

Value Value::operator - (const Value &v) const
{
	if (is_nan || v.is_nan)
		return Value();
	if (is_vector && v.is_vector)
		return Value(x - v.x, y - v.y, z - v.z);
	if (!is_vector && !v.is_vector)
		return Value(x - v.x);
	return Value();
}

Value Value::operator * (const Value &v) const
{
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

Value Value::operator / (const Value &v) const
{
	if (is_nan || v.is_nan || is_vector || v.is_vector)
		return Value();
	return Value(x / v.x);
}

Value Value::operator % (const Value &v) const
{
	if (is_nan || v.is_nan || is_vector || v.is_vector)
		return Value();
	return Value(fmod(x, v.x));
}

Value Value::inv() const
{
	if (is_nan)
		return Value();
	if (is_vector)
		return Value(-x, -y, -z);
	return Value(-x);
}

QString Value::dump() const
{
	if (is_nan)
		return QString("NaN");
	if (is_vector) {
		QString text;
		text.sprintf("[%f %f %f]", x, y, z);
		return text;
	}
	QString text;
	text.sprintf("%f", x);
	return text;
}

