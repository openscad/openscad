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

#include "expression.h"
#include "value.h"
#include "context.h"
#include <assert.h>
#include <sstream>
#include <algorithm>
#include "stl-utils.h"

Expression::Expression()
{
	this->const_value = NULL;
}

Expression::~Expression()
{
	std::for_each(this->children.begin(), this->children.end(), del_fun<Expression>());
	delete this->const_value;
}

Value Expression::evaluate(const Context *context) const
{
	if (this->type == "!")
		return ! this->children[0]->evaluate(context);
	if (this->type == "&&")
		return this->children[0]->evaluate(context) && this->children[1]->evaluate(context);
	if (this->type == "||")
		return this->children[0]->evaluate(context) || this->children[1]->evaluate(context);
	if (this->type == "*")
		return this->children[0]->evaluate(context) * this->children[1]->evaluate(context);
	if (this->type == "/")
		return this->children[0]->evaluate(context) / this->children[1]->evaluate(context);
	if (this->type == "%")
		return this->children[0]->evaluate(context) % this->children[1]->evaluate(context);
	if (this->type == "+")
		return this->children[0]->evaluate(context) + this->children[1]->evaluate(context);
	if (this->type == "-")
		return this->children[0]->evaluate(context) - this->children[1]->evaluate(context);
	if (this->type == "<")
		return this->children[0]->evaluate(context) < this->children[1]->evaluate(context);
	if (this->type == "<=")
		return this->children[0]->evaluate(context) <= this->children[1]->evaluate(context);
	if (this->type == "==")
		return this->children[0]->evaluate(context) == this->children[1]->evaluate(context);
	if (this->type == "!=")
		return this->children[0]->evaluate(context) != this->children[1]->evaluate(context);
	if (this->type == ">=")
		return this->children[0]->evaluate(context) >= this->children[1]->evaluate(context);
	if (this->type == ">")
		return this->children[0]->evaluate(context) > this->children[1]->evaluate(context);
	if (this->type == "?:") {
		Value v = this->children[0]->evaluate(context);
		if (v.type == Value::BOOL)
			return this->children[v.b ? 1 : 2]->evaluate(context);
		return Value();
	}
	if (this->type == "[]") {
		Value v1 = this->children[0]->evaluate(context);
		Value v2 = this->children[1]->evaluate(context);
		if (v1.type == Value::VECTOR && v2.type == Value::NUMBER) {
			int i = int(v2.num);
			if (i >= 0 && i < int(v1.vec.size()))
				return *v1.vec[i];
		}
		return Value();
	}
	if (this->type == "I")
		return this->children[0]->evaluate(context).inv();
	if (this->type == "C")
		return *this->const_value;
	if (this->type == "R") {
		Value v1 = this->children[0]->evaluate(context);
		Value v2 = this->children[1]->evaluate(context);
		Value v3 = this->children[2]->evaluate(context);
		if (v1.type == Value::NUMBER && v2.type == Value::NUMBER && v3.type == Value::NUMBER) {
			Value r = Value();
			r.type = Value::RANGE;
			r.range_begin = v1.num;
			r.range_step = v2.num;
			r.range_end = v3.num;
			return r;
		}
		return Value();
	}
	if (this->type == "V") {
		Value v;
		v.type = Value::VECTOR;
		for (size_t i = 0; i < this->children.size(); i++)
			v.append(new Value(this->children[i]->evaluate(context)));
		return v;
	}
	if (this->type == "L")
		return context->lookup_variable(this->var_name);
	if (this->type == "N")
	{
		Value v = this->children[0]->evaluate(context);

		if (v.type == Value::VECTOR && this->var_name == "x")
			return *v.vec[0];
		if (v.type == Value::VECTOR && this->var_name == "y")
			return *v.vec[1];
		if (v.type == Value::VECTOR && this->var_name == "z")
			return *v.vec[2];

		if (v.type == Value::RANGE && this->var_name == "begin")
			return Value(v.range_begin);
		if (v.type == Value::RANGE && this->var_name == "step")
			return Value(v.range_step);
		if (v.type == Value::RANGE && this->var_name == "end")
			return Value(v.range_end);

		return Value();
	}
	if (this->type == "F") {
		std::vector<Value> argvalues;
		for (size_t i=0; i < this->children.size(); i++)
			argvalues.push_back(this->children[i]->evaluate(context));
		return context->evaluate_function(this->call_funcname, this->call_argnames, argvalues);
	}
	abort();
}

std::string Expression::toString() const
{
	std::stringstream stream;

	if (this->type == "*" || this->type == "/" || this->type == "%" || this->type == "+" ||
			this->type == "-" || this->type == "<" || this->type == "<=" || this->type == "==" || 
			this->type == "!=" || this->type == ">=" || this->type == ">") {
		stream << "(" << *this->children[0] << " " << this->type << " " << *this->children[1] << ")";
	}
	else if (this->type == "?:") {
		stream << "(" << *this->children[0] << " ? " << this->type << " : " << *this->children[1] << ")";
	}
	else if (this->type == "[]") {
		stream << "(" << *this->children[0] << "[" << *this->children[1] << "])";
	}
	else if (this->type == "I") {
		stream << "(-" << *this->children[0] << ")";
	}
	else if (this->type == "C") {
		stream << *this->const_value;
	}
	else if (this->type == "R") {
		stream << "[" << *this->children[0] << " : " << *this->children[1] << " : " << this->children[2] << "]";
	}
	else if (this->type == "V") {
		stream << "[";
		for (size_t i=0; i < this->children.size(); i++) {
			if (i > 0) stream << ", ";
			stream << *this->children[i];
		}
		stream << "]";
	}
	else if (this->type == "L") {
		stream << this->var_name;
	}
	else if (this->type == "N") {
		stream << "(" << *this->children[0] << "." << this->var_name << ")";
	}
	else if (this->type == "F") {
		stream << this->call_funcname << "(";
		for (size_t i=0; i < this->children.size(); i++) {
			if (i > 0) stream << ", ";
			if (!this->call_argnames[i].empty()) stream << this->call_argnames[i]  << " = ";
			stream << *this->children[i];
		}
		stream << ")";
	}
	else {
		assert(false && "Illegal expression type");
	}

	return stream.str();
}

std::ostream &operator<<(std::ostream &stream, const Expression &expr)
{
	stream << expr.toString();
	return stream;
}
