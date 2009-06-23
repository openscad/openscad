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

Value::Value()
{
	reset_undef();
}

Value::Value(bool v)
{
	reset_undef();
	type = BOOL;
	b = v;
}

Value::Value(double v)
{
	reset_undef();
	type = NUMBER;
	num = v;
}

Value::Value(double v1, double v2, double v3)
{
	reset_undef();
	type = VECTOR;
	x = v1;
	y = v2;
	z = v3;
}

Value::Value(double m[16])
{
	reset_undef();
	type = MATRIX;
	for (int i=0; i<16; i++)
		this->m[i] = m[i];
}

Value::Value(const QString &t)
{
	reset_undef();
	type = STRING;
	text = t;
}

Value::Value(const Value &v)
{
	reset_undef();
	type = v.type;
	b = v.b;
	num = v.num;
	x = v.x;
	y = v.y;
	z = v.z;
	for (int i=0; i<16; i++)
		m[i] = v.m[i];
	text = v.text;
}

Value& Value::operator = (const Value &v)
{
	reset_undef();
	type = v.type;
	b = v.b;
	num = v.num;
	x = v.x;
	y = v.y;
	z = v.z;
	for (int i=0; i<16; i++)
		m[i] = v.m[i];
	text = v.text;
	return *this;
}

Value Value::operator + (const Value &v) const
{
	if (type == VECTOR && v.type == VECTOR) {
		return Value(x + v.x, y + v.y, z + v.z);
	}
	if (type == MATRIX && v.type == MATRIX) {
		double m_[16];
		for (int i=0; i<16; i++)
			m_[i] = m[i] + v.m[i];
		return Value(m);
	}
	if (type == NUMBER && v.type == NUMBER) {
		return Value(num + v.num);
	}
	return Value();
}

Value Value::operator - (const Value &v) const
{
	if (type == VECTOR && v.type == VECTOR) {
		return Value(x + v.x, y + v.y, z + v.z);
	}
	if (type == MATRIX && v.type == MATRIX) {
		double m_[16];
		for (int i=0; i<16; i++)
			m_[i] = m[i] + v.m[i];
		return Value(m);
	}
	if (type == NUMBER && v.type == NUMBER) {
		return Value(num + v.num);
	}
	return Value();
}

Value Value::operator * (const Value &v) const
{
	if (type == VECTOR && v.type == VECTOR) {
		double nx = (y-v.y)*(z-v.z) - (z-v.z)*(y-v.y);
		double ny = (z-v.z)*(x-v.x) - (x-v.x)*(z-v.z);
		double nz = (x-v.x)*(y-v.y) - (y-v.y)*(x-v.x);
		return Value(nx, ny, nz);
	}
	if (type == VECTOR && v.type == NUMBER) {
		return Value(x * v.num, y * v.num, z * v.num);
	}
	if (type == NUMBER && v.type == VECTOR) {
		return Value(num * v.x, num * v.y, num * v.z);
	}
	if (type == NUMBER && v.type == NUMBER) {
		return Value(num * v.num);
	}
	return Value();
}

Value Value::operator / (const Value &v) const
{
	if (type == NUMBER && v.type == NUMBER) {
		return Value(num / v.num);
	}
	return Value();
}

Value Value::operator % (const Value &v) const
{
	if (type == NUMBER && v.type == NUMBER) {
		return Value(fmod(num, v.num));
	}
	return Value();
}

Value Value::inv() const
{
	if (type == MATRIX) {
		double m_[16];
		for (int i=0; i<16; i++)
			m_[i] = -m[i];
		return Value(m);
	}
	if (type == VECTOR)
		return Value(-x, -y, -z);
	if (type == NUMBER)
		return Value(-x);
	return Value();
}

QString Value::dump() const
{
	if (type == STRING) {
		return QString("\"") + text + QString("\"");
	}
	if (type == MATRIX) {
		QString text = "[";
		for (int i=0; i<16; i++) {
			QString t;
			t.sprintf("%f", m[i]);
			if (i % 4 == 0 && i > 0)
				text += ";";
			if (i > 0)
				text += " ";
			text += t;
		}
		text += "]";
		return text;
	}
	if (type == VECTOR) {
		QString text;
		text.sprintf("[%f %f %f]", x, y, z);
		return text;
	}
	if (type == RANGE) {
		QString text;
		text.sprintf("[ %f : %f : %f ]", r_begin, r_step, r_end);
		return text;
	}
	if (type == NUMBER) {
		QString text;
		text.sprintf("%f", num);
		return text;
	}
	if (type == BOOL) {
		return QString(b ? "true" : "false");
	}
	return QString("undef");
}

void Value::reset_undef()
{
	type = UNDEFINED;
	b = false;
	num = 0;
	r_begin = 0;
	r_step = 0;
	r_end = 0;
	x = 0;
	y = 0;
	z = 0;
	for (int i=0; i<16; i++)
		m[i] = 0;
	text = QString();
}

