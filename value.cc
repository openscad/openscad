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

Value::~Value()
{
	for (int i = 0; i < vec.size(); i++)
		delete vec[i];
	vec.clear();
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

Value::Value(const QString &t)
{
	reset_undef();
	type = STRING;
	text = t;
}

Value::Value(const Value &v)
{
	*this = v;
}

Value& Value::operator = (const Value &v)
{
	reset_undef();
	type = v.type;
	b = v.b;
	num = v.num;
	for (int i = 0; i < v.vec.size(); i++)
		vec.append(new Value(*v.vec[i]));
	range_begin = v.range_begin;
	range_step = v.range_step;
	range_end = v.range_end;
	text = v.text;
	return *this;
}

Value Value::operator + (const Value &v) const
{
	if (type == VECTOR && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < vec.size() && i < v.vec.size(); i++)
			r.vec.append(new Value(*vec[i] + *v.vec[i]));
		return r;
	}
	if (type == NUMBER && v.type == NUMBER) {
		return Value(num + v.num);
	}
	return Value();
}

Value Value::operator - (const Value &v) const
{
	if (type == VECTOR && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < vec.size() && i < v.vec.size(); i++)
			r.vec.append(new Value(*vec[i] - *v.vec[i]));
		return r;
	}
	if (type == NUMBER && v.type == NUMBER) {
		return Value(num + v.num);
	}
	return Value();
}

Value Value::operator * (const Value &v) const
{
	if (type == VECTOR && v.type == NUMBER) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < vec.size(); i++)
			r.vec.append(new Value(*vec[i] * v));
		return r;
	}
	if (type == NUMBER && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < v.vec.size(); i++)
			r.vec.append(new Value(v * *v.vec[i]));
		return r;
	}
	if (type == NUMBER && v.type == NUMBER) {
		return Value(num * v.num);
	}
	return Value();
}

Value Value::operator / (const Value &v) const
{
	if (type == VECTOR && v.type == NUMBER) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < vec.size(); i++)
			r.vec.append(new Value(*vec[i] / v));
		return r;
	}
	if (type == NUMBER && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < v.vec.size(); i++)
			r.vec.append(new Value(v / *v.vec[i]));
		return r;
	}
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
	if (type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < vec.size(); i++)
			r.vec.append(new Value(vec[i]->inv()));
		return r;
	}
	if (type == NUMBER)
		return Value(-num);
	return Value();
}

bool Value::getnum(double &v) const
{
	if (type != NUMBER)
		return false;
	v = num;
	return true;
}

bool Value::getv3(double &x, double &y, double &z) const
{
	if (type != VECTOR || vec.size() != 3)
		return false;
	if (vec[0]->type != NUMBER)
		return false;
	if (vec[1]->type != NUMBER)
		return false;
	if (vec[2]->type != NUMBER)
		return false;
	x = vec[0]->num;	
	y = vec[1]->num;	
	z = vec[2]->num;	
	return true;
}

QString Value::dump() const
{
	if (type == STRING) {
		return QString("\"") + text + QString("\"");
	}
	if (type == VECTOR) {
		QString text = "[";
		for (int i = 0; i < vec.size(); i++) {
			if (i > 0)
				text += ", ";
			text += vec[i]->dump();
		}
		return text + "]";
	}
	if (type == RANGE) {
		QString text;
		text.sprintf("[ %f : %f : %f ]", range_begin, range_step, range_end);
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
	for (int i = 0; i < vec.size(); i++)
		delete vec[i];
	vec.clear();
	range_begin = 0;
	range_step = 0;
	range_end = 0;
	text = QString();
}

