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

#include "value.h"
#include "mathc99.h"


Value::Value()
{
	reset_undef();
}

Value::~Value()
{
	for (int i = 0; i < this->vec.size(); i++)
		delete this->vec[i];
	this->vec.clear();
}

Value::Value(bool v)
{
	reset_undef();
	this->type = BOOL;
	this->b = v;
}

Value::Value(double v)
{
	reset_undef();
	this->type = NUMBER;
	this->num = v;
}

Value::Value(const QString &t)
{
	reset_undef();
	this->type = STRING;
	this->text = t;
}

Value::Value(const Value &v)
{
	*this = v;
}

Value& Value::operator = (const Value &v)
{
	reset_undef();
	this->type = v.type;
	this->b = v.b;
	this->num = v.num;
	for (int i = 0; i < v.vec.size(); i++)
		this->vec.append(new Value(*v.vec[i]));
	this->range_begin = v.range_begin;
	this->range_step = v.range_step;
	this->range_end = v.range_end;
	this->text = v.text;
	return *this;
}

Value Value::operator ! () const
{
	if (this->type == BOOL) {
		return Value(!this->b);
	}
	return Value();
}

Value Value::operator && (const Value &v) const
{
	if (this->type == BOOL && v.type == BOOL) {
		return Value(this->b && v.b);
	}
	return Value();
}

Value Value::operator || (const Value &v) const
{
	if (this->type == BOOL && v.type == BOOL) {
		return Value(this->b || v.b);
	}
	return Value();
}

Value Value::operator + (const Value &v) const
{
	if (this->type == VECTOR && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < this->vec.size() && i < v.vec.size(); i++)
			r.vec.append(new Value(*this->vec[i] + *v.vec[i]));
		return r;
	}
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num + v.num);
	}
	return Value();
}

Value Value::operator - (const Value &v) const
{
	if (this->type == VECTOR && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < this->vec.size() && i < v.vec.size(); i++)
			r.vec.append(new Value(*this->vec[i] - *v.vec[i]));
		return r;
	}
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num - v.num);
	}
	return Value();
}

Value Value::operator * (const Value &v) const
{
	if (this->type == VECTOR && v.type == NUMBER) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < this->vec.size(); i++)
			r.vec.append(new Value(*this->vec[i] * v));
		return r;
	}
	if (this->type == NUMBER && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < v.vec.size(); i++)
			r.vec.append(new Value(*this * *v.vec[i]));
		return r;
	}
	if (this->type == VECTOR && v.type == VECTOR) { // dot product
		double num = 0;
		for (int i = 0; i < this->vec.size() && i < v.vec.size(); i++){
			if (this->vec[i]->type == NUMBER && v.vec[i]->type == NUMBER)
				num += (this->vec[i]->num)*(v.vec[i]->num);
		}
		return Value(num);
	}
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num * v.num);
	}
	return Value();
}

Value Value::operator / (const Value &v) const
{
	if (this->type == VECTOR && v.type == NUMBER) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < this->vec.size(); i++)
			r.vec.append(new Value(*this->vec[i] / v));
		return r;
	}
	if (this->type == NUMBER && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < v.vec.size(); i++)
			r.vec.append(new Value(v / *v.vec[i]));
		return r;
	}
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num / v.num);
	}
	return Value();
}

Value Value::operator % (const Value &v) const
{
	
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(fmod(this->num, v.num));
	}
	if (this->type == VECTOR && v.type == VECTOR) { // Cross Product
		if ( this->vec.size() < 2 || v.vec.size() < 2
		  || this->vec.size() > 3 || v.vec.size() > 3)
			return Value();
		for (int i = 0; i < v.vec.size(); i++){
			if (v.vec[i]->type != NUMBER) 
				return Value();
		}
		for (int i = 0; i < this->vec.size(); i++){
			if (this->vec[i]->type != NUMBER) 
				return Value();
		}
		Value r;
		r.type = VECTOR;
		if (this->vec.size() == 2 || v.vec.size() == 2){
			r.vec.append(new Value(0.));
			r.vec.append(new Value(0.));
			r.vec.append(new Value((this->vec[0]->num)*(v.vec[1]->num) - (this->vec[1]->num)*(v.vec[0]->num)));
			return r;
		}
		r.vec.append(new Value((this->vec[1]->num)*(v.vec[2]->num) - (this->vec[2]->num)*(v.vec[1]->num)));
		r.vec.append(new Value((this->vec[2]->num)*(v.vec[0]->num) - (this->vec[0]->num)*(v.vec[2]->num)));
		r.vec.append(new Value((this->vec[0]->num)*(v.vec[1]->num) - (this->vec[1]->num)*(v.vec[0]->num)));
		return r;		
		
	}
	return Value();
}

Value Value::operator < (const Value &v) const
{
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num < v.num);
	}
	return Value();
}

Value Value::operator <= (const Value &v) const
{
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num <= v.num);
	}
	return Value();
}

Value Value::operator == (const Value &v) const
{
	if (this->type == BOOL && v.type == BOOL) {
		return Value(this->b == v.b);
	}
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num == v.num);
	}
	if (this->type == RANGE && v.type == RANGE) {
		return Value(this->range_begin == v.range_begin && this->range_step == v.range_step && this->range_end == v.range_end);
	}
	if (this->type == VECTOR && v.type == VECTOR) {
		if (this->vec.size() != v.vec.size())
			return Value(false);
		for (int i=0; i<this->vec.size(); i++)
			if (!(*this->vec[i] == *v.vec[i]).b)
				return Value(false);
		return Value(true);
	}
	if (this->type == STRING && v.type == STRING) {
		return Value(this->text == v.text);
	}
	return Value(false);
}

Value Value::operator != (const Value &v) const
{
	Value eq = *this == v;
	return Value(!eq.b);
}

Value Value::operator >= (const Value &v) const
{
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num >= v.num);
	}
	return Value();
}

Value Value::operator > (const Value &v) const
{
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num > v.num);
	}
	return Value();
}

Value Value::inv() const
{
	if (this->type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (int i = 0; i < this->vec.size(); i++)
			r.vec.append(new Value(this->vec[i]->inv()));
		return r;
	}
	if (this->type == NUMBER)
		return Value(-this->num);
	return Value();
}

bool Value::getnum(double &v) const
{
	if (this->type != NUMBER)
		return false;
	v = this->num;
	return true;
}

bool Value::getv2(double &x, double &y) const
{
	if (this->type != VECTOR || this->vec.size() != 2)
		return false;
	if (this->vec[0]->type != NUMBER)
		return false;
	if (this->vec[1]->type != NUMBER)
		return false;
	x = this->vec[0]->num;	
	y = this->vec[1]->num;	
	return true;
}

bool Value::getv3(double &x, double &y, double &z) const
{
	if (this->type == VECTOR && this->vec.size() == 2) {
		if (getv2(x, y)) {
			z = 0;
			return true;
		}
		return false;
	}
	if (this->type != VECTOR || this->vec.size() != 3)
		return false;
	if (this->vec[0]->type != NUMBER)
		return false;
	if (this->vec[1]->type != NUMBER)
		return false;
	if (this->vec[2]->type != NUMBER)
		return false;
	x = this->vec[0]->num;	
	y = this->vec[1]->num;	
	z = this->vec[2]->num;	
	return true;
}

QString Value::dump() const
{
	if (this->type == STRING) {
		return QString("\"") + this->text + QString("\"");
	}
	if (this->type == VECTOR) {
		QString text = "[";
		for (int i = 0; i < this->vec.size(); i++) {
			if (i > 0)
				text += ", ";
			text += this->vec[i]->dump();
		}
		return text + "]";
	}
	if (this->type == RANGE) {
		QString text;
		text.sprintf("[ %g : %g : %g ]", this->range_begin, this->range_step, this->range_end);
		return text;
	}
	if (this->type == NUMBER) {
		QString text;
		text.sprintf("%g", this->num);
		return text;
	}
	if (this->type == BOOL) {
		return QString(this->b ? "true" : "false");
	}
	return QString("undef");
}

void Value::reset_undef()
{
	this->type = UNDEFINED;
	this->b = false;
	this->num = 0;
	for (int i = 0; i < this->vec.size(); i++)
		delete this->vec[i];
	this->vec.clear();
	this->range_begin = 0;
	this->range_step = 0;
	this->range_end = 0;
	this->text = QString();
}
