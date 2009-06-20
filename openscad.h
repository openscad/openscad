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

#ifndef OPENSCAD_H
#define OPENSCAD_H

#include <QHash>
#include <QVector>

class Value;
class Expression;

class AbstractFunction;
class BuiltinFunction;
class Function;

class AbstractModule;
class ModuleInstanciation;
class Module;

class Context;
class AbstractNode;

class Value
{
public:
	double x, y, z;
	bool is_vector;
	bool is_nan;

	Value() : x(0), y(0), z(0), is_vector(false), is_nan(true) { }
	Value(double v1) : x(v1), y(0), z(0), is_vector(false), is_nan(false) { }
	Value(double v1, double v2, double v3) : x(v1), y(v2), z(v3), is_vector(true), is_nan(false) { }
	Value(const Value &v) : x(v.x), y(v.y), z(v.z), is_vector(v.is_vector), is_nan(v.is_nan) { }

	Value& operator = (const Value &v);
	Value operator + (const Value &v) const;
	Value operator - (const Value &v) const;
	Value operator * (const Value &v) const;
	Value operator / (const Value &v) const;
	Value operator % (const Value &v) const;
	Value inv() const;
};

class Expression
{
public:
	QVector<Expression*> children;

	Value const_value;
	QString var_name;

	QString call_funcname;
	QVector<QString> call_argnames;

	// Math operators: * / % + -
	// Invert (prefix '-'): I
	// Constant value: C
	// Variable: V
	// Function call: F
	char type;

	Expression();
	~Expression();

	Value evaluate(Context *context);
};

class AbstractFunction
{
public:
	virtual ~AbstractFunction();
	virtual Value evaluate(Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);
};

class BuiltinFunction : public AbstractFunction
{
public:
	typedef Value (*eval_func_t)(const QVector<Value> &args);
	eval_func_t eval_func;

	BuiltinFunction(eval_func_t f) : eval_func(f) { }
	virtual ~BuiltinFunction();

	virtual Value evaluate(Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);
};

class Function : public AbstractFunction
{
public:
	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	Expression expr;

	Function() { }
	virtual ~Function();

	virtual Value evaluate(Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);
};

extern QHash<QString, AbstractFunction*> builtin_functions;
extern void initialize_builtin_functions();
extern void destroy_builtin_functions();

class AbstractModule
{
public:
	virtual ~AbstractModule();
	virtual AbstractNode *evaluate(Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);
};

class ModuleInstanciation
{
public:
	QString label;
	QString modname;
	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	ModuleInstanciation() { }
	~ModuleInstanciation();
};

class Module : public AbstractModule
{
public:
	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	QHash<QString, Expression*> assignments;
	QHash<QString, AbstractFunction*> functions;
	QHash<QString, AbstractModule*> modules;

	QVector<ModuleInstanciation> children;

	Module() { }
	virtual ~Module();

	virtual AbstractNode *evaluate(Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);
};

extern QHash<QString, AbstractModule*> builtin_modules;
extern void initialize_builtin_modules();
extern void destroy_builtin_modules();

class Context
{
public:
	Context *parent;
	QHash<QString, Value> variables;
	QHash<QString, AbstractFunction*> *functions_p;
	QHash<QString, AbstractModule*> *modules_p;

	Context(Context *parent) : parent(parent) { }
	void args(const QVector<QString> &argnames, const QVector<Expression*> &argexpr, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);

	Value lookup_variable(QString name);
	Value evaluate_function(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues);
	AbstractNode *evaluate_module(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues);
};

class AbstractNode
{
public:
	QVector<AbstractNode*> children;
};

#endif

