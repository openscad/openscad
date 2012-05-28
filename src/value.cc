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
#include <assert.h>
#include <sstream>
#include <QDir>
#include <boost/foreach.hpp>
#include "printutils.h"

Value::Value()
{
	reset_undef();
}

Value::~Value()
{
	for (size_t i = 0; i < this->vec.size(); i++) delete this->vec[i];
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

Value::Value(const std::string &t)
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
	for (size_t i = 0; i < v.vec.size(); i++) {
		this->vec.push_back(new Value(*v.vec[i]));
	}
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
		for (size_t i = 0; i < this->vec.size() && i < v.vec.size(); i++)
			r.vec.push_back(new Value(*this->vec[i] + *v.vec[i]));
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
		for (size_t i = 0; i < this->vec.size() && i < v.vec.size(); i++)
			r.vec.push_back(new Value(*this->vec[i] - *v.vec[i]));
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
		for (size_t i = 0; i < this->vec.size(); i++)
			r.vec.push_back(new Value(*this->vec[i] * v));
		return r;
	}
	if (this->type == NUMBER && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (size_t i = 0; i < v.vec.size(); i++)
			r.vec.push_back(new Value(*this * *v.vec[i]));
		return r;
	}
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num * v.num);
	}
	if (this->type == VECTOR && v.type == VECTOR && this->vec.size() == v.vec.size() ) {
	  if ( this->vec[0]->type == NUMBER && v.vec[0]->type == NUMBER ) {
		// Vector dot product.
		double r=0.0;
		for (size_t i=0; i <this->vec.size(); i++) {
		  if ( this->vec[i]->type != NUMBER || v.vec[i]->type != NUMBER ) return Value();
		  r = r + (this->vec[i]->num * v.vec[i]->num);
		}
		return Value(r);
	  } else if ( this->vec[0]->type == VECTOR && v.vec[0]->type == NUMBER ) {
		// Matrix * Vector
		Value r;
		r.type = VECTOR;
		for ( size_t i=0; i < this->vec.size(); i++) {
		  double r_e=0.0;
		  if ( this->vec[i]->vec.size() != v.vec.size() ) return Value();
		  for ( size_t j=0; j < this->vec[i]->vec.size(); j++) {
		    if ( this->vec[i]->vec[j]->type != NUMBER || v.vec[i]->type != NUMBER ) return Value();
		    r_e = r_e + (this->vec[i]->vec[j]->num * v.vec[j]->num);
		  }
		  r.vec.push_back(new Value(r_e));
		}
		return r;
	  } else if (this->vec[0]->type == NUMBER && v.vec[0]->type == VECTOR ) {
		// Vector * Matrix
		Value r;
		r.type = VECTOR;
		for ( size_t i=0; i < v.vec[0]->vec.size(); i++) {
		  double r_e=0.0;
		  for ( size_t j=0; j < v.vec.size(); j++) {
		    if ( v.vec[j]->vec.size() != v.vec[0]->vec.size() ) return Value();
		    if ( this->vec[j]->type != NUMBER || v.vec[j]->vec[i]->type != NUMBER ) return Value();
		    r_e = r_e + (this->vec[j]->num * v.vec[j]->vec[i]->num);
		  }
		  r.vec.push_back(new Value(r_e));
		}
		return r;
	  }
	}
	if (this->type == VECTOR && v.type == VECTOR &&  this->vec[0]->type == VECTOR && v.vec[0]->type == VECTOR && this->vec[0]->vec.size() == v.vec.size() ) {
		// Matrix * Matrix
		Value rrow;
		rrow.type = VECTOR;
		for ( size_t i=0; i < this->vec.size(); i++ ) {
		  Value * rcol=new Value();
		  rcol->type = VECTOR;
		  for ( size_t j=0; j < this->vec.size(); j++ ) {
		    double r_e=0.0;
		    for ( size_t k=0; k < v.vec.size(); k++ ) {
		      r_e = r_e + (this->vec[i]->vec[k]->num * v.vec[k]->vec[j]->num);
		    }
		    // PRINTB("  r_e = %s",r_e);
		    rcol->vec.push_back(new Value(r_e));
		  }
		  rrow.vec.push_back(rcol);
		}
		return rrow;
	}
	return Value();
}

Value Value::operator / (const Value &v) const
{
	if (this->type == VECTOR && v.type == NUMBER) {
		Value r;
		r.type = VECTOR;
		for (size_t i = 0; i < this->vec.size(); i++)
			r.vec.push_back(new Value(*this->vec[i] / v));
		return r;
	}
	if (this->type == NUMBER && v.type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (size_t i = 0; i < v.vec.size(); i++)
			r.vec.push_back(new Value(v / *v.vec[i]));
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
	return Value();
}

Value Value::operator < (const Value &v) const
{
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num < v.num);
	}
	else if (this->type == STRING && v.type == STRING) {
		return Value(this->text < v.text);
	}
	return Value();
}

Value Value::operator <= (const Value &v) const
{
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num <= v.num);
	}
	else if (this->type == STRING && v.type == STRING) {
		return Value(this->text <= v.text);
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
		for (size_t i=0; i<this->vec.size(); i++)
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
	else if (this->type == STRING && v.type == STRING) {
		return Value(this->text >= v.text);
	}
	return Value();
}

Value Value::operator > (const Value &v) const
{
	if (this->type == NUMBER && v.type == NUMBER) {
		return Value(this->num > v.num);
	}
	else if (this->type == STRING && v.type == STRING) {
		return Value(this->text > v.text);
	}
	return Value();
}

Value Value::inv() const
{
	if (this->type == VECTOR) {
		Value r;
		r.type = VECTOR;
		for (size_t i = 0; i < this->vec.size(); i++)
			r.vec.push_back(new Value(this->vec[i]->inv()));
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

bool Value::getv3(double &x, double &y, double &z, double defaultval) const
{
	if (this->type == VECTOR && this->vec.size() == 2) {
		if (getv2(x, y)) {
			z = defaultval;
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

void Value::reset_undef()
{
	this->type = UNDEFINED;
	this->b = false;
	this->num = 0;
	for (size_t i = 0; i < this->vec.size(); i++) delete this->vec[i];
	this->vec.clear();
	this->range_begin = 0;
	this->range_step = 0;
	this->range_end = 0;
	this->text = "";
}

std::string Value::toString() const
{
	std::stringstream stream;
	stream.precision(16);

	switch (this->type) {
	case STRING:
		stream << this->text;
		break;
	case VECTOR:
		stream << '[';
		for (size_t i = 0; i < this->vec.size(); i++) {
			if (i > 0) stream << ", ";
			stream << *(this->vec[i]);
		}
		stream << ']';
		break;
	case RANGE:
		stream	<< '['
			<< this->range_begin
			<< " : "
			<< this->range_step
			<< " : "
			<< this->range_end
			<< ']';
		break;
	case NUMBER:
#ifdef OPENSCAD_TESTING
		// Quick and dirty hack to work around floating point rounding differences
		// across platforms for testing purposes.
	{
		if (this->num != this->num) { // Fix for avoiding nan vs. -nan across platforms
			stream << "nan";
			break;
		}
		std::stringstream tmp;
		tmp.precision(12);
		tmp.setf(std::ios_base::fixed);
		tmp << this->num;
		std::string tmpstr = tmp.str();
		size_t endpos = tmpstr.find_last_not_of('0');
		if (endpos >= 0 && tmpstr[endpos] == '.') endpos--;
		tmpstr = tmpstr.substr(0, endpos+1);
		size_t dotpos = tmpstr.find('.');
		if (dotpos != std::string::npos) {
			if (tmpstr.size() - dotpos > 12) tmpstr.erase(dotpos + 12);
		}
		stream << tmpstr;
	}
#else
		stream << this->num;
#endif
		break;
	case BOOL:
		stream << (this->b ? "true" : "false");
		break;
	default:
		stream << "undef";
	}

	return stream.str();
}

bool Value::toBool() const
{
	switch (this->type) {
	case STRING:
		return this->text.size() > 0;
		break;
	case VECTOR:
		return this->vec.size() > 0;
		break;
	case RANGE:
		return true;
		break;
	case NUMBER:
		return this->num != 0;
		break;
	case BOOL:
		return this->b;
		break;
	default:
		return false;
		break;
	}
}

/*!
	Append a value to this vector.
	This must be of type VECTOR.
*/
void Value::append(Value *val)
{
	assert(this->type == VECTOR);
	this->vec.push_back(val);
}

std::ostream &operator<<(std::ostream &stream, const Value &value)
{
	if (value.type == Value::STRING) stream << QuotedString(value.toString());
	else stream << value.toString();
	return stream;
}

std::ostream &operator<<(std::ostream &stream, const Filename &filename)
{
	stream << QuotedString(QDir::current().relativeFilePath(QString::fromStdString(filename)).toStdString());
	return stream;
}

// FIXME: This could probably be done more elegantly using boost::regex
std::ostream &operator<<(std::ostream &stream, const QuotedString &s)
{
	stream << '"';
	BOOST_FOREACH(char c, s) {
		switch (c) {
		case '\t':
			stream << "\\t";
			break;
		case '\n':
			stream << "\\n";
			break;
		case '"':
		case '\\':
			stream << '\\';
			stream << c;
			break;
		default:
			stream << c;
		}
	}
	stream << '"';
	return stream;
}
