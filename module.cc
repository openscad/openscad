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

AbstractModule::~AbstractModule()
{
}

AbstractNode *AbstractModule::evaluate(const Context*, const QVector<QString>&, const QVector<Value>&) const
{
	return NULL;
}

QString AbstractModule::dump(QString indent, QString name) const
{
	return QString("%1abstract module %2();\n").arg(indent, name);
}

ModuleInstanciation::~ModuleInstanciation()
{
	foreach (Expression *v, argexpr)
		delete v;
	foreach (ModuleInstanciation *v, children)
		delete v;
}

QString ModuleInstanciation::dump(QString indent) const
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

AbstractNode *ModuleInstanciation::evaluate(const Context*) const
{
	/* FIXME */
	return NULL;
}

Module::~Module()
{
	foreach (Expression *v, assignments)
		delete v;
	foreach (AbstractFunction *v, functions)
		delete v;
	foreach (AbstractModule *v, modules)
		delete v;
	foreach (ModuleInstanciation *v, children)
		delete v;
}

AbstractNode *Module::evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const
{
	Context c(ctx);
	c.args(argnames, argexpr, call_argnames, call_argvalues);

	/* FIXME */
	return NULL;
}

QString Module::dump(QString indent, QString name) const
{
	QString text = QString("%1module %2(").arg(indent, name);
	for (int i=0; i < argnames.size(); i++) {
		if (i > 0)
			text += QString(", ");
		text += argnames[i];
		if (argexpr[i])
			text += QString(" = ") + argexpr[i]->dump();
	}
	text += QString(") {\n");
	{
		QHashIterator<QString, Expression*> i(assignments);
		while (i.hasNext()) {
			i.next();
			text += QString("%1\t%2 = %3;\n").arg(indent, i.key(), i.value()->dump());
		}
	}
	{
		QHashIterator<QString, AbstractFunction*> i(functions);
		while (i.hasNext()) {
			i.next();
			text += i.value()->dump(indent + QString("\t"), i.key());
		}
	}
	{
		QHashIterator<QString, AbstractModule*> i(modules);
		while (i.hasNext()) {
			i.next();
			text += i.value()->dump(indent + QString("\t"), i.key());
		}
	}
	for (int i = 0; i < children.size(); i++) {
		text += children[i]->dump(indent + QString("\t"));
	}
	text += QString("%1}\n").arg(indent);
	return text;
}

QHash<QString, AbstractModule*> builtin_modules;

void initialize_builtin_modules()
{
	/* FIXME */
}

void destroy_builtin_modules()
{
	foreach (AbstractModule *v, builtin_modules)
		delete v;
	builtin_modules.clear();
}

