/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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
			if (i < v1.vec.size())
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
			v.vec.append(new Value(children[i]->evaluate(context)));
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

QString Expression::dump() const
{
	if (type == "*" || type == "/" || type == "%" || type == "+" || type == "-" ||
			type == "<" || type == "<=" || type == "==" || type == "!=" || type == ">=" || type == ">")
		return QString("(%1 %2 %3)").arg(children[0]->dump(), QString(type), children[1]->dump());
	if (type == "?:")
		return QString("(%1 ? %2 : %3)").arg(children[0]->dump(), children[1]->dump(), children[2]->dump());
	if (type == "[]")
		return QString("(%1[%2])").arg(children[0]->dump(), children[1]->dump());
	if (type == "I")
		return QString("(-%1)").arg(children[0]->dump());
	if (type == "C")
		return const_value->dump();
	if (type == "R")
		return QString("[%1 : %2 : %3]").arg(children[0]->dump(), children[1]->dump(), children[2]->dump());
	if (type == "V") {
		QString text = QString("[");
		for (int i=0; i < children.size(); i++) {
			if (i > 0)
				text += QString(", ");
			text += children[i]->dump();
		}
		return text + QString("]");
	}
	if (type == "L")
		return var_name;
	if (type == "N")
		return QString("(%1.%2)").arg(children[0]->dump(), var_name);
	if (type == "F") {
		QString text = call_funcname + QString("(");
		for (int i=0; i < children.size(); i++) {
			if (i > 0)
				text += QString(", ");
			if (!call_argnames[i].isEmpty())
				text += call_argnames[i] + QString(" = ");
			text += children[i]->dump();
		}
		return text + QString(")");
	}
	abort();
}

