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
	case 'I':
		return children[0]->evaluate(context).inv();
	case 'C':
		return const_value;
	case 'V':
		return Value(children[0]->evaluate(context), children[1]->evaluate(context), children[2]->evaluate(context));
	case 'L':
		return context->lookup_variable(var_name);
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
		return QString("(%1%2%3)").arg(children[0]->dump(), QString(type), children[1]->dump());
	case 'I':
		return QString("(-%1)").arg(children[0]->dump());
	case 'C':
		return const_value.dump();
	case 'V':
		return QString("[%1, %2, %3]").arg(children[0]->dump(), children[1]->dump(), children[2]->dump());
	case 'L':
		return var_name;
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
		}
	default:
		abort();
	}
}

