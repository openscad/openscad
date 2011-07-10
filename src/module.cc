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

#include "module.h"
#include "node.h"
#include "context.h"
#include "expression.h"
#include "function.h"
#include "builtin.h"
#include "printutils.h"

AbstractModule::~AbstractModule()
{
}

AbstractNode *AbstractModule::evaluate(const Context*, const ModuleInstantiation *inst) const
{
	AbstractNode *node = new AbstractNode(inst);

	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n)
			node->children.append(n);
	}

	return node;
}

QString AbstractModule::dump(QString indent, QString name) const
{
	return QString("%1abstract module %2();\n").arg(indent, name);
}

ModuleInstantiation::~ModuleInstantiation()
{
	foreach (Expression *v, argexpr)
		delete v;
	foreach (ModuleInstantiation *v, children)
		delete v;
}

IfElseModuleInstantiation::~IfElseModuleInstantiation()
{
	foreach (ModuleInstantiation *v, else_children)
		delete v;
}

QString ModuleInstantiation::dump(QString indent) const
{
	QString text = indent;
	if (!label.isEmpty())
		text += label + QString(": ");
	text += modname + QString("(");
	for (int i=0; i < argnames.size(); i++) {
		if (i > 0)
			text += QString(", ");
		if (!argnames[i].isEmpty())
			text += argnames[i] + QString(" = ");
		text += argexpr[i]->dump();
	}
	if (children.size() == 0) {
		text += QString(");\n");
	} else if (children.size() == 1) {
		text += QString(")\n");
		text += children[0]->dump(indent + QString("\t"));
	} else {
		text += QString(") {\n");
		for (int i = 0; i < children.size(); i++) {
			text += children[i]->dump(indent + QString("\t"));
		}
		text += QString("%1}\n").arg(indent);
	}
	return text;
}

AbstractNode *ModuleInstantiation::evaluate(const Context *ctx) const
{
	AbstractNode *node = NULL;
	if (this->ctx) {
		PRINTA("WARNING: Ignoring recursive module instanciation of '%1'.", modname);
	} else {
		ModuleInstantiation *that = (ModuleInstantiation*)this;
		that->argvalues.clear();
		foreach (Expression *v, that->argexpr) {
			that->argvalues.append(v->evaluate(ctx));
		}
		that->ctx = ctx;
		node = ctx->evaluate_module(this);
		that->ctx = NULL;
		that->argvalues.clear();
	}
	return node;
}

Module::~Module()
{
	foreach (Expression *v, assignments_expr)
		delete v;
	foreach (AbstractFunction *v, functions)
		delete v;
	foreach (AbstractModule *v, modules)
		delete v;
	foreach (ModuleInstantiation *v, children)
		delete v;
}

AbstractNode *Module::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	c.inst_p = inst;
	c.set_variable("$children", Value(double(inst->children.size())));

	c.functions_p = &functions;
	c.modules_p = &modules;

	if (!usedlibs.empty())
		c.usedlibs_p = &usedlibs;
	else
		c.usedlibs_p = NULL;

	for (int i = 0; i < assignments_var.size(); i++) {
		c.set_variable(assignments_var[i], assignments_expr[i]->evaluate(&c));
	}

	AbstractNode *node = new AbstractNode(inst);
	for (int i = 0; i < children.size(); i++) {
		AbstractNode *n = children[i]->evaluate(&c);
		if (n != NULL)
			node->children.append(n);
	}

	return node;
}

QString Module::dump(QString indent, QString name) const
{
	QString text, tab;
	if (!name.isEmpty()) {
		text = QString("%1module %2(").arg(indent, name);
		for (int i=0; i < argnames.size(); i++) {
			if (i > 0)
				text += QString(", ");
			text += argnames[i];
			if (argexpr[i])
				text += QString(" = ") + argexpr[i]->dump();
		}
		text += QString(") {\n");
		tab = "\t";
	}
	{
		QHashIterator<QString, AbstractFunction*> i(functions);
		while (i.hasNext()) {
			i.next();
			text += i.value()->dump(indent + tab, i.key());
		}
	}
	{
		QHashIterator<QString, AbstractModule*> i(modules);
		while (i.hasNext()) {
			i.next();
			text += i.value()->dump(indent + tab, i.key());
		}
	}
	for (int i = 0; i < assignments_var.size(); i++) {
		text += QString("%1%2 = %3;\n").arg(indent + tab, assignments_var[i], assignments_expr[i]->dump());
	}
	for (int i = 0; i < children.size(); i++) {
		text += children[i]->dump(indent + tab);
	}
	if (!name.isEmpty()) {
		text += QString("%1}\n").arg(indent);
	}
	return text;
}

QHash<QString, AbstractModule*> builtin_modules;

void initialize_builtin_modules()
{
	builtin_modules["group"] = new AbstractModule();

	register_builtin_csgops();
	register_builtin_transform();
	register_builtin_primitives();
	register_builtin_surface();
	register_builtin_control();
	register_builtin_render();
	register_builtin_import();
	register_builtin_projection();
	register_builtin_cgaladv();
	register_builtin_dxf_linear_extrude();
	register_builtin_dxf_rotate_extrude();
	register_builtin_viewport();
}

void destroy_builtin_modules()
{
	foreach (AbstractModule *v, builtin_modules)
		delete v;
	builtin_modules.clear();
}
