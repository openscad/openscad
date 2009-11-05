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

AbstractFunction::~AbstractFunction()
{
}

Value AbstractFunction::evaluate(const Context*, const QVector<QString>&, const QVector<Value>&) const
{
	return Value();
}

QString AbstractFunction::dump(QString indent, QString name) const
{
	return QString("%1abstract function %2();\n").arg(indent, name);
}

Function::~Function()
{
	for (int i=0; i < argexpr.size(); i++)
		delete argexpr[i];
	delete expr;
}

Value Function::evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const
{
	Context c(ctx);
	c.args(argnames, argexpr, call_argnames, call_argvalues);
	if (expr)
		return expr->evaluate(&c);
	return Value();
}

QString Function::dump(QString indent, QString name) const
{
	QString text = QString("%1function %2(").arg(indent, name);
	for (int i=0; i < argnames.size(); i++) {
		if (i > 0)
			text += QString(", ");
		text += argnames[i];
		if (argexpr[i])
			text += QString(" = ") + argexpr[i]->dump();
	}
	text += QString(") = %1;\n").arg(expr->dump());
	return text;
}

QHash<QString, AbstractFunction*> builtin_functions;

BuiltinFunction::~BuiltinFunction()
{
}

Value BuiltinFunction::evaluate(const Context*, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const
{
	return eval_func(call_argnames, call_argvalues);
}

QString BuiltinFunction::dump(QString indent, QString name) const
{
	return QString("%1builtin function %2();\n").arg(indent, name);
}

static double deg2rad(double x)
{
	while (x < 0.0)
		x += 360.0;
	while (x >= 360.0)
		x -= 360.0;
	x = x * M_PI * 2.0 / 360.0;
	return x;
}

static double rad2deg(double x)
{
	x = x * 360.0 / (M_PI * 2.0);
	while (x < 0.0)
		x += 360.0;
	while (x >= 360.0)
		x -= 360.0;
	return x;
}

Value builtin_sin(const QVector<QString>&, const QVector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(sin(deg2rad(args[0].num)));
	return Value();
}

Value builtin_cos(const QVector<QString>&, const QVector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(cos(deg2rad(args[0].num)));
	return Value();
}

Value builtin_asin(const QVector<QString>&, const QVector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(rad2deg(asin(args[0].num)));
	return Value();
}

Value builtin_acos(const QVector<QString>&, const QVector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(rad2deg(acos(args[0].num)));
	return Value();
}

Value builtin_tan(const QVector<QString>&, const QVector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(tan(deg2rad(args[0].num)));
	return Value();
}

Value builtin_atan(const QVector<QString>&, const QVector<Value> &args)
{
	if (args.size() == 1 && args[0].type == Value::NUMBER)
		return Value(rad2deg(atan(args[0].num)));
	return Value();
}

Value builtin_atan2(const QVector<QString>&, const QVector<Value> &args)
{
	if (args.size() == 2 && args[0].type == Value::NUMBER && args[1].type == Value::NUMBER)
		return Value(rad2deg(atan2(args[0].num, args[1].num)));
	return Value();
}

Value builtin_pow(const QVector<QString>&, const QVector<Value> &args)
{
	if (args.size() == 2 && args[0].type == Value::NUMBER && args[1].type == Value::NUMBER)
		return Value(pow(args[0].num, args[1].num));
	return Value();
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
	builtin_functions["pow"] = new BuiltinFunction(&builtin_pow);
	initialize_builtin_dxf_dim();
}

void destroy_builtin_functions()
{
	foreach (AbstractFunction *v, builtin_functions)
		delete v;
	builtin_functions.clear();
}

