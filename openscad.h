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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <fstream>
#include <iostream>

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
	enum type_e {
		UNDEFINED,
		BOOL,
		NUMBER,
		RANGE,
		VECTOR,
		MATRIX,
		STRING
	};

	enum type_e type;

	bool b;
	double num;
	double x, y, z;
	double r_begin;
	double r_step;
	double r_end;
	double m[16];
	QString text;

	Value();
	Value(bool v);
	Value(double v);
	Value(double v1, double v2, double v3);
	Value(double m[16]);
	Value(const QString &t);

	Value(const Value &v);
	Value& operator = (const Value &v);

	Value operator + (const Value &v) const;
	Value operator - (const Value &v) const;
	Value operator * (const Value &v) const;
	Value operator / (const Value &v) const;
	Value operator % (const Value &v) const;
	Value inv() const;

	QString dump() const;

private:
	void reset_undef();
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
	// Condition (?: operator): ?
	// Invert (prefix '-'): I
	// Constant value: C
	// Create Range: R
	// Create Vector: V
	// Create Matrix: M
	// Lookup Variable: L
	// Lookup member per name: N
	// Function call: F
	char type;

	Expression();
	~Expression();

	Value evaluate(const Context *context) const;
	QString dump() const;
};

class AbstractFunction
{
public:
	virtual ~AbstractFunction();
	virtual Value evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const;
	virtual QString dump(QString indent, QString name) const;
};

class BuiltinFunction : public AbstractFunction
{
public:
	typedef Value (*eval_func_t)(const QVector<Value> &args);
	eval_func_t eval_func;

	BuiltinFunction(eval_func_t f) : eval_func(f) { }
	virtual ~BuiltinFunction();

	virtual Value evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const;
	virtual QString dump(QString indent, QString name) const;
};

class Function : public AbstractFunction
{
public:
	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	Expression *expr;

	Function() { }
	virtual ~Function();

	virtual Value evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const;
	virtual QString dump(QString indent, QString name) const;
};

extern QHash<QString, AbstractFunction*> builtin_functions;
extern void initialize_builtin_functions();
extern void destroy_builtin_functions();

class AbstractModule
{
public:
	virtual ~AbstractModule();
	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
	virtual QString dump(QString indent, QString name) const;
};

class ModuleInstanciation
{
public:
	QString label;
	QString modname;
	QVector<QString> argnames;
	QVector<Expression*> argexpr;
	QVector<ModuleInstanciation*> children;

	ModuleInstanciation() { }
	~ModuleInstanciation();

	QString dump(QString indent) const;
	AbstractNode *evaluate(const Context *ctx) const;
};

class Module : public AbstractModule
{
public:
	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	QVector<QString> assignments_var;
	QVector<Expression*> assignments_expr;

	QHash<QString, AbstractFunction*> functions;
	QHash<QString, AbstractModule*> modules;

	QVector<ModuleInstanciation*> children;

	Module() { }
	virtual ~Module();

	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
	virtual QString dump(QString indent, QString name) const;
};

extern QHash<QString, AbstractModule*> builtin_modules;
extern void initialize_builtin_modules();
extern void destroy_builtin_modules();

extern void register_builtin_csg();
extern void register_builtin_trans();
extern void register_builtin_primitive();

class Context
{
public:
	const Context *parent;
	QHash<QString, Value> variables;
	const QHash<QString, AbstractFunction*> *functions_p;
	const QHash<QString, AbstractModule*> *modules_p;

	Context(const Context *parent) : parent(parent) { }
	void args(const QVector<QString> &argnames, const QVector<Expression*> &argexpr, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);

	Value lookup_variable(QString name) const;
	Value evaluate_function(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues) const;
	AbstractNode *evaluate_module(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues, const QVector<AbstractNode*> child_nodes) const;
};

// The CGAL template magic slows down the compilation process by a factor of 5.
// So we only include the declaration of AbstractNode where it is needed...
#ifdef INCLUDE_ABSTRACT_NODE_DETAILS

#include <CGAL/Gmpq.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>

typedef CGAL::Cartesian<CGAL::Gmpq> CGAL_Kernel;
// typedef CGAL::Extended_cartesian<CGAL::Gmpq> CGAL_Kernel;
typedef CGAL::Polyhedron_3<CGAL_Kernel> CGAL_Polyhedron;
typedef CGAL_Polyhedron::HalfedgeDS CGAL_HDS;
typedef CGAL::Polyhedron_incremental_builder_3<CGAL_HDS> CGAL_Polybuilder;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel> CGAL_Nef_polyhedron;
typedef CGAL_Nef_polyhedron::Aff_transformation_3  CGAL_Aff_transformation;
typedef CGAL_Nef_polyhedron::Vector_3 CGAL_Vector;
typedef CGAL_Nef_polyhedron::Plane_3 CGAL_Plane;
typedef CGAL_Nef_polyhedron::Point_3 CGAL_Point;

class AbstractNode
{
public:
	QVector<AbstractNode*> children;

	int progress_mark;
	void progress_prepare();
	void progress_report() const;

	virtual ~AbstractNode();
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
	virtual QString dump(QString indent) const;
};

extern int progress_report_count;
extern void (*progress_report_f)(const class AbstractNode*, void*, int);
extern void *progress_report_vp;

void progress_report_prep(AbstractNode *root, void (*f)(const class AbstractNode *node, void *vp, int mark), void *vp);
void progress_report_fin();

#endif /* HIDE_ABSTRACT_NODE_DETAILS */

extern AbstractModule *parse(FILE *f, int debug);

#endif

