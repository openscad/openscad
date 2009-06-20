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

AbstractNode *AbstractModule::evaluate(Context*, const QVector<QString>&, const QVector<Value>&)
{
	return NULL;
}

ModuleInstanciation::~ModuleInstanciation()
{
	foreach (Expression *v, argexpr)
		delete v;
}

Module::~Module()
{
	foreach (Expression *v, assignments)
		delete v;
	foreach (AbstractFunction *v, functions)
		delete v;
	foreach (AbstractModule *v, modules)
		delete v;
}

AbstractNode *Module::evaluate(Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues)
{
	Context c(ctx);
	c.args(argnames, argexpr, call_argnames, call_argvalues);

	/* FIXME */

	return NULL;
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

