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

Expression::Expression()
{
	type = 0;
}

Expression::~Expression()
{
	for (int i=0; i < children.size(); i++)
		delete children[i];
}

Value Expression::evaluate(const Context *context) const
{
	switch (type)
	{
	case '*':
		return children[0]->evaluate(context) * children[1]->evaluate(context);
	case '/':
		return children[0]->evaluate(context) / children[1]->evaluate(context);
	case '%':
		return children[0]->evaluate(context) % children[1]->evaluate(context);
	case '+':
		return children[0]->evaluate(context) + children[1]->evaluate(context);
	case '-':
		return children[0]->evaluate(context) - children[1]->evaluate(context);
	case '?':
		{
			Value v = children[0]->evaluate(context);
			if (v.type == Value::BOOL)
				return children[v.b ? 1 : 2]->evaluate(context);
			return Value();
		}
	case 'I':
		return children[0]->evaluate(context).inv();
	case 'C':
		return const_value;
	case 'R':
		{
			Value v1 = children[0]->evaluate(context);
			Value v2 = children[1]->evaluate(context);
			Value v3 = children[2]->evaluate(context);
			if (v1.type == Value::NUMBER && v2.type == Value::NUMBER && v3.type == Value::NUMBER) {
				Value r = Value();
				r.type = Value::RANGE;
				r.r_begin = v1.num;
				r.r_step = v2.num;
				r.r_end = v3.num;
				return r;
			}
			return Value();
		}
	case 'V':
		{
			Value v1 = children[0]->evaluate(context);
			Value v2 = children[1]->evaluate(context);
			Value v3 = children[2]->evaluate(context);
			if (v1.type == Value::NUMBER && v2.type == Value::NUMBER && v3.type == Value::NUMBER)
				return Value(v1.num, v2.num, v3.num);
			return Value();
		}
	case 'M':
		{
			double m[16];
			for (int i=0; i<16; i++) {
				Value v = children[i]->evaluate(context);
				if (v.type != Value::NUMBER)
					return Value();
				m[i == 15 ? 15 : (i*4) % 15] = v.num;
			}
			return Value(m);
		}
	case 'L':
		return context->lookup_variable(var_name);
	case 'N':
		{
			Value v = children[0]->evaluate(context);

			if (v.type == Value::VECTOR && var_name == QString("x"))
				return Value(v.x);
			if (v.type == Value::VECTOR && var_name == QString("y"))
				return Value(v.y);
			if (v.type == Value::VECTOR && var_name == QString("z"))
				return Value(v.z);

			if (v.type == Value::RANGE && var_name == QString("begin"))
				return Value(v.r_begin);
			if (v.type == Value::RANGE && var_name == QString("step"))
				return Value(v.r_step);
			if (v.type == Value::RANGE && var_name == QString("end"))
				return Value(v.r_end);

			for (int i=0; i<16; i++) {
				QString n;
				n.sprintf("m%d", i+1);
				if (v.type == Value::MATRIX && var_name == n)
					return Value(v.m[i]);
			}

			return Value();
		}
	case 'F':
		{
			QVector<Value> argvalues;
			for (int i=0; i < children.size(); i++)
				argvalues.append(children[i]->evaluate(context));
			return context->evaluate_function(call_funcname, call_argnames, argvalues);
		}
	default:
		abort();
	}
}

QString Expression::dump() const
{
	switch (type)
	{
	case '*':
	case '/':
	case '%':
	case '+':
	case '-':
		return QString("(%1 %2 %3)").arg(children[0]->dump(), QString(type), children[1]->dump());
	case '?':
		return QString("(%1 ? %2 : %3)").arg(children[0]->dump(), children[1]->dump(), children[2]->dump());
	case 'I':
		return QString("(-%1)").arg(children[0]->dump());
	case 'C':
		return const_value.dump();
	case 'R':
		return QString("[%1 : %2 : %3]").arg(children[0]->dump(), children[1]->dump(), children[2]->dump());
	case 'V':
		return QString("[%1, %2, %3]").arg(children[0]->dump(), children[1]->dump(), children[2]->dump());
	case 'M':
		{
			QString text = "[";
			for (int i = 0; i < 16; i++) {
				if (i % 4 == 0 && i > 0)
					text += ";";
				if (i > 0)
					text += " ";
				text += children[i]->dump();
			}
			text += "]";
			return text;
		}
	case 'L':
		return var_name;
	case 'N':
		return QString("(%1.%2)").arg(children[0]->dump(), var_name);
	case 'F':
		{
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
	default:
		abort();
	}
}

