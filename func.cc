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
#include <math.h>

AbstractFunction::~AbstractFunction()
{
}

Value AbstractFunction::evaluate(Context*, const QVector<QString>&, const QVector<Value>&)
{
	return Value();
}

Function::~Function()
{
	for (int i=0; i < argexpr.size(); i++)
		delete argexpr[i];
}

Value Function::evaluate(Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues)
{
	Context c(ctx);
	c.args(argnames, argexpr, call_argnames, call_argvalues);
	return expr.evaluate(&c);
}

QHash<QString, AbstractFunction*> builtin_functions;

BuiltinFunction::~BuiltinFunction()
{
}

Value BuiltinFunction::evaluate(Context*, const QVector<QString>&, const QVector<Value> &call_argvalues)
{
	return eval_func(call_argvalues);
}

Value builtin_sin(const QVector<Value> &args)
{
	if (args[0].is_nan || args[0].is_vector)
		return Value();
	return Value(sin(args[0].x));
}

Value builtin_cos(const QVector<Value> &args)
{
	if (args[0].is_nan || args[0].is_vector)
		return Value();
	return Value(cos(args[0].x));
}

Value builtin_asin(const QVector<Value> &args)
{
	if (args[0].is_nan || args[0].is_vector)
		return Value();
	return Value(asin(args[0].x));
}

Value builtin_acos(const QVector<Value> &args)
{
	if (args[0].is_nan || args[0].is_vector)
		return Value();
	return Value(acos(args[0].x));
}

Value builtin_tan(const QVector<Value> &args)
{
	if (args[0].is_nan || args[0].is_vector)
		return Value();
	return Value(tan(args[0].x));
}

Value builtin_atan(const QVector<Value> &args)
{
	if (args[0].is_nan || args[0].is_vector)
		return Value();
	return Value(atan(args[0].x));
}

Value builtin_atan2(const QVector<Value> &args)
{
	if (args[0].is_nan || args[0].is_vector || args[1].is_nan || args[1].is_vector)
		return Value();
	return Value(atan2(args[0].x, args[1].x));
}

void initialize_builtin_functions()
{
	builtin_functions["sin"] = new BuiltinFunction(&builtin_sin);
	builtin_functions["cos"] = new BuiltinFunction(&builtin_cos);
	builtin_functions["asin"] = new BuiltinFunction(&builtin_asin);
	builtin_functions["acos"] = new BuiltinFunction(&builtin_acos);
	builtin_functions["tan"] = new BuiltinFunction(&builtin_tan);
	builtin_functions["atan"] = new BuiltinFunction(&builtin_atan);
	builtin_functions["atan2"] = new BuiltinFunction(&builtin_atan2);
}

void destroy_builtin_functions()
{
	foreach (AbstractFunction *v, builtin_functions)
		delete v;
	builtin_functions.clear();
}

