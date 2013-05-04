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
#include "ModuleCache.h"
#include "node.h"
#include "modcontext.h"
#include "evalcontext.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "boosty.h"
#include <boost/foreach.hpp>
#include <sstream>
#include <sys/stat.h>

AbstractModule::~AbstractModule()
{
}

AbstractNode *AbstractModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	AbstractNode *node = new AbstractNode(inst);

	node->children = inst->instantiateChildren(evalctx);

	return node;
}

std::string AbstractModule::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	dump << indent << "abstract module " << name << "();\n";
	return dump.str();
}

ModuleInstantiation::~ModuleInstantiation()
{
	BOOST_FOREACH(const Assignment &arg, this->arguments) delete arg.second;
}

IfElseModuleInstantiation::~IfElseModuleInstantiation()
{
}

/*!
	Returns the absolute path to the given filename, unless it's empty.
 */
std::string ModuleInstantiation::getAbsolutePath(const std::string &filename) const
{
	if (!filename.empty() && !boosty::is_absolute(fs::path(filename))) {
		return boosty::absolute(fs::path(this->modpath) / filename).string();
	}
	else {
		return filename;
	}
}

std::string ModuleInstantiation::dump(const std::string &indent) const
{
	std::stringstream dump;
	dump << indent;
	dump << modname + "(";
	for (size_t i=0; i < this->arguments.size(); i++) {
		const Assignment &arg = this->arguments[i];
		if (i > 0) dump << ", ";
		if (!arg.first.empty()) dump << arg.first << " = ";
		dump << *arg.second;
	}
	if (scope.numElements() == 0) {
		dump << ");\n";
	} else if (scope.numElements() == 1) {
		dump << ")\n";
		dump << scope.dump(indent + "\t");
	} else {
		dump << ") {\n";
		scope.dump(indent + "\t");
		dump << indent << "}\n";
	}
	return dump.str();
}

AbstractNode *ModuleInstantiation::evaluate(const Context *ctx) const
{
	EvalContext c(ctx, this->arguments, &this->scope);

#if 0 && DEBUG
	PRINT("New eval ctx:");
	c.dump(NULL, this);
#endif
	AbstractNode *node = ctx->instantiate_module(*this, &c); // Passes c as evalctx
	return node;
}

std::vector<AbstractNode*> ModuleInstantiation::instantiateChildren(const Context *evalctx) const
{
	return this->scope.instantiateChildren(evalctx);
}

std::vector<AbstractNode*> IfElseModuleInstantiation::instantiateElseChildren(const Context *evalctx) const
{
	return this->else_scope.instantiateChildren(evalctx);
}

Module::~Module()
{
}

AbstractNode *Module::instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	ModuleContext c(ctx, evalctx);
	c.initializeModule(*this);
	c.set_variable("$children", Value(double(inst->scope.children.size())));
	// FIXME: Set document path to the path of the module
#if 0 && DEBUG
	c.dump(this, inst);
#endif

	AbstractNode *node = new AbstractNode(inst);
	std::vector<AbstractNode *> instantiatednodes = this->scope.instantiateChildren(&c);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string Module::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	std::string tab;
	if (!name.empty()) {
		dump << indent << "module " << name << "(";
		for (size_t i=0; i < this->definition_arguments.size(); i++) {
			const Assignment &arg = this->definition_arguments[i];
			if (i > 0) dump << ", ";
			dump << arg.first;
			if (arg.second) dump << " = " << *arg.second;
		}
		dump << ") {\n";
		tab = "\t";
	}
	dump << scope.dump(indent + tab);
	if (!name.empty()) {
		dump << indent << "}\n";
	}
	return dump.str();
}

void FileModule::registerInclude(const std::string &filename)
{
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	stat(filename.c_str(), &st);
	this->includes[filename] = st.st_mtime;
}

/*!
	Check if any dependencies have been modified and recompile them.
	Returns true if anything was recompiled.
*/
bool FileModule::handleDependencies()
{
	if (this->is_handling_dependencies) return false;
	this->is_handling_dependencies = true;

	bool changed = false;
	// Iterating manually since we want to modify the container while iterating
	FileModule::ModuleContainer::iterator iter = this->usedlibs.begin();
	while (iter != this->usedlibs.end()) {
		FileModule::ModuleContainer::iterator curr = iter++;
		FileModule *oldmodule = curr->second;
		curr->second = ModuleCache::instance()->evaluate(curr->first);
		if (curr->second != oldmodule) {
			changed = true;
#ifdef DEBUG
			PRINTB_NOCACHE("  %s: %p", curr->first % curr->second);
#endif
		}
		if (!curr->second) {
			PRINTB_NOCACHE("WARNING: Failed to compile library '%s'.", curr->first);
			this->usedlibs.erase(curr);
		}
	}

	this->is_handling_dependencies = false;
	return changed;
}

AbstractNode *FileModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	assert(evalctx == NULL);
	FileContext c(*this, ctx);
	c.initializeModule(*this);
	// FIXME: Set document path to the path of the module
#if 0 && DEBUG
	c.dump(this, inst);
#endif

	AbstractNode *node = new AbstractNode(inst);
	std::vector<AbstractNode *> instantiatednodes = this->scope.instantiateChildren(ctx, &c);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}
