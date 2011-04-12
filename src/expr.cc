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

Expression::Expression()
{
	const_value = NULL;
}

Expression::~Expression()
{
	for (int i=0; i < children.size(); i++)
		delete children[i];
	if (const_value)
		delete const_value;
}

Value Expression::evaluate(const Context *context) const
{
	if (type == "!")
		return ! children[0]->evaluate(context);
	if (type == "&&")
		return children[0]->evaluate(context) && children[1]->evaluate(context);
	if (type == "||")
		return children[0]->evaluate(context) || children[1]->evaluate(context);
	if (type == "*")
		return children[0]->evaluate(context) * children[1]->evaluate(context);
	if (type == "/")
		return children[0]->evaluate(context) / children[1]->evaluate(context);
	if (type == "%")
		return children[0]->evaluate(context) % children[1]->evaluate(context);
	if (type == "+")
		return children[0]->evaluate(context) + children[1]->evaluate(context);
	if (type == "-")
		return children[0]->evaluate(context) - children[1]->evaluate(context);
	if (type == "<")
		return children[0]->evaluate(context) < children[1]->evaluate(context);
	if (type == "<=")
		return children[0]->evaluate(context) <= children[1]->evaluate(context);
	if (type == "==")
		return children[0]->evaluate(context) == children[1]->evaluate(context);
	if (type == "!=")
		return children[0]->evaluate(context) != children[1]->evaluate(context);
	if (type == ">=")
		return children[0]->evaluate(context) >= children[1]->evaluate(context);
	if (type == ">")
		return children[0]->evaluate(context) > children[1]->evaluate(context);
	if (type == "?:") {
		Value v = children[0]->evaluate(context);
		if (v.type == Value::BOOL)
			return children[v.b ? 1 : 2]->evaluate(context);
		return Value();
	}
	if (type == "[]") {
		Value v1 = children[0]->evaluate(context);
		Value v2 = children[1]->evaluate(context);
		if (v1.type == Value::VECTOR && v2.type == Value::NUMBER) {
			int i = (int)(v2.num);
			if (i >= 0 && i < v1.vec.size())
				return *v1.vec[i];
		}
		return Value();
	}
	if (type == "I")
		return children[0]->evaluate(context).inv();
	if (type == "C")
		return *const_value;
	if (type == "R") {
		Value v1 = children[0]->evaluate(context);
		Value v2 = children[1]->evaluate(context);
		Value v3 = children[2]->evaluate(context);
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
	if (type == "V") {
		Value v;
		v.type = Value::VECTOR;
		for (int i = 0; i < children.size(); i++)
			v.append(new Value(children[i]->evaluate(context)));
		return v;
	}
	if (type == "L")
		return context->lookup_variable(var_name);
	if (type == "N")
	{
		Value v = children[0]->evaluate(context);

		if (v.type == Value::VECTOR && var_name == QString("x"))
			return *v.vec[0];
		if (v.type == Value::VECTOR && var_name == QString("y"))
			return *v.vec[1];
		if (v.type == Value::VECTOR && var_name == QString("z"))
			return *v.vec[2];

		if (v.type == Value::RANGE && var_name == QString("begin"))
			return Value(v.range_begin);
		if (v.type == Value::RANGE && var_name == QString("step"))
			return Value(v.range_step);
		if (v.type == Value::RANGE && var_name == QString("end"))
			return Value(v.range_end);

		return Value();
	}
	if (type == "F") {
		QVector<Value> argvalues;
		for (int i=0; i < children.size(); i++)
			argvalues.append(children[i]->evaluate(context));
		return context->evaluate_function(call_funcname, call_argnames, argvalues);
	}
	abort();
}

std::string Expression::toString() const
{
	std::stringstream stream;

	if (this->type == "*" || this->type == "/" || this->type == "%" || this->type == "+" ||
			this->type == "-" || this->type == "<" || this->type == "<=" || this->type == "==" || 
			this->type == "!=" || this->type == ">=" || this->type == ">") {
		stream << "(" << *children[0] << " " << this->type << " " << *children[1] << ")";
	}
	else if (this->type == "?:") {
		stream << "(" << *children[0] << " ? " << this->type << " : " << *children[1] << ")";
	}
	else if (this->type == "[]") {
		stream << "(" << *children[0] << "[" << *children[1] << "])";
	}
	else if (this->type == "I") {
		stream << "(-" << *children[0] << ")";
	}
	else if (this->type == "C") {
		stream << *const_value;
	}
	else if (this->type == "R") {
		stream << "[" << *children[0] << " : " << *children[1] << " : " << children[2] << "]";
	}
	else if (this->type == "V") {
		stream << "[";
		for (int i=0; i < children.size(); i++) {
			if (i > 0) stream << ", ";
			stream << *children[i];
		}
		stream << "]";
	}
	else if (this->type == "L") {
		stream << var_name;
	}
	else if (this->type == "N") {
		stream << "(" << *children[0] << "." << var_name << ")";
	}
	else if (this->type == "F") {
		stream << call_funcname << "(";
		for (int i=0; i < children.size(); i++) {
			if (i > 0) stream << ", ";
			if (!call_argnames[i].isEmpty()) stream << call_argnames[i]  << " = ";
			stream << *children[i];
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
