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

Value Expression::evaluate(Context *context)
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
		return context->lookup_variable(var_name);
	case 'F':
		{
			QVector<Value> argvalues;
			for (int i=0; i < children.size(); i++)
				argvalues.append(children[i]->evaluate(context));
			return context->evaluate_function(call_funcname, call_argnames, argvalues);
		}
	default:
		return Value();
	}
}

