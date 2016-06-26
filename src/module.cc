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
#include "ModuleInstantiation.h"
#include "ModuleCache.h"
#include "node.h"
#include "builtin.h"
#include "modcontext.h"
#include "evalcontext.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "parsersettings.h"
#include "exceptions.h"
#include "stackcheck.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "boosty.h"
#include "FontCache.h"
#include <sstream>
#include <sys/stat.h>

AbstractModule::~AbstractModule()
{
}

AbstractNode *GroupModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	(void)ctx; // avoid unusued parameter warning

	AbstractNode *node = new GroupNode(inst);

	node->children = inst->instantiateChildren(evalctx);

	return node;
}

double AbstractModule::lookup_double_variable_with_default(Context &c, std::string variable, double def) const
{
	ValuePtr v = c.lookup_variable(variable, true);
	return (v->type() == Value::NUMBER) ? v->toDouble() : def;
}

std::string AbstractModule::lookup_string_variable_with_default(Context &c, std::string variable, std::string def) const
{
	ValuePtr v = c.lookup_variable(variable, true);
	return (v->type() == Value::STRING) ? v->toString() : def;
}

std::string AbstractModule::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	dump << indent << "abstract module " << name << "();\n";
	return dump.str();
}

std::deque<std::string> UserModule::module_stack;

AbstractNode *UserModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	if (StackCheck::inst()->check()) {
		throw RecursionException::create("module", inst->name());
		return NULL;
	}

	// At this point we know that nobody will modify the dependencies of the local scope
	// passed to this instance, so we can populate the context
	inst->scope.apply(*evalctx);
    
	ModuleContext c(ctx, evalctx);
	// set $children first since we might have variables depending on it
	c.set_variable("$children", ValuePtr(double(inst->scope.children.size())));
	module_stack.push_back(inst->name());
	c.set_variable("$parent_modules", ValuePtr(double(module_stack.size())));
	c.initializeModule(*this);
	// FIXME: Set document path to the path of the module
#if 0 && DEBUG
	c.dump(this, inst);
#endif

	AbstractNode *node = new GroupNode(inst);
	std::vector<AbstractNode *> instantiatednodes = this->scope.instantiateChildren(&c);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	module_stack.pop_back();

	return node;
}

std::string UserModule::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	std::string tab;
	if (!name.empty()) {
		dump << indent << "module " << name << "(";
		for (size_t i=0; i < this->definition_arguments.size(); i++) {
			const Assignment &arg = this->definition_arguments[i];
			if (i > 0) dump << ", ";
			dump << arg.name;
			if (arg.expr) dump << " = " << *arg.expr;
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

FileModule::~FileModule()
{
	delete context;
}

std::string FileModule::dump(const std::string &indent, const std::string &name) const
{
	return scope.dump(indent);
}

void FileModule::registerUse(const std::string path) {
	std::string extraw = boosty::extension_str(fs::path(path));
	std::string ext = boost::algorithm::to_lower_copy(extraw);
	
	if ((ext == ".otf") || (ext == ".ttf")) {
		if (fs::is_regular(path)) {
			FontCache::instance()->register_font_file(path);
		} else {
			PRINTB("ERROR: Can't read font with path '%s'", path);
		}
	} else {
		usedlibs.insert(path);
	}
}

void FileModule::registerInclude(const std::string &localpath,
																 const std::string &fullpath)
{
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	bool valid = stat(fullpath.c_str(), &st) == 0;
	IncludeFile inc = {fullpath, valid, st.st_mtime};
	this->includes[localpath] = inc;
}

bool FileModule::includesChanged() const
{
	for(const auto &item : this->includes) {
		if (include_modified(item.second)) return true;
	}
	return false;
}

bool FileModule::include_modified(const IncludeFile &inc) const
{
	struct stat st;
	memset(&st, 0, sizeof(struct stat));

	fs::path fullpath = find_valid_path(this->path, inc.filename);
	bool valid = !fullpath.empty() ? (stat(boosty::stringy(fullpath).c_str(), &st) == 0) : false;
	
	if (valid && !inc.valid) return true; // Detect appearance of file but not removal
	if (valid && st.st_mtime > inc.mtime) return true;
	
	return false;
}

/*!
	Check if any dependencies have been modified and recompile them.
	Returns true if anything was recompiled.
*/
bool FileModule::handleDependencies()
{
	if (this->is_handling_dependencies) return false;
	this->is_handling_dependencies = true;

	bool somethingchanged = false;
	std::vector<std::pair<std::string,std::string>> updates;

	// If a lib in usedlibs was previously missing, we need to relocate it
	// by searching the applicable paths. We can identify a previously missing module
	// as it will have a relative path.
	for(auto filename : this->usedlibs) {

		bool wasmissing = false;
		bool found = true;

		// Get an absolute filename for the module
		if (!boosty::is_absolute(filename)) {
			wasmissing = true;
			fs::path fullpath = find_valid_path(this->path, filename);
			if (!fullpath.empty()) {
				updates.push_back(std::make_pair(filename, boosty::stringy(fullpath)));
				filename = boosty::stringy(fullpath);
			}
			else {
				found = false;
			}
		}

		if (found) {
			bool wascached = ModuleCache::instance()->isCached(filename);
			FileModule *oldmodule = ModuleCache::instance()->lookup(filename);
			FileModule *newmodule;
			bool changed = ModuleCache::instance()->evaluate(filename, newmodule);
			// Detect appearance but not removal of files, and keep old module
			// on compile errors (FIXME: Is this correct behavior?)
			if (changed) {
				PRINTDB("  %s: %p -> %p", filename % oldmodule % newmodule);
			}
			somethingchanged |= changed;
			// Only print warning if we're not part of an automatic reload
			if (!newmodule && !wascached && !wasmissing) {
				PRINTB_NOCACHE("WARNING: Failed to compile library '%s'.", filename);
			}
		}
	}

	// Relative filenames which were located is reinserted as absolute filenames
	typedef std::pair<std::string,std::string> stringpair;
	for(const auto &files : updates) {
		this->usedlibs.erase(files.first);
		this->usedlibs.insert(files.second);
	}
	this->is_handling_dependencies = false;
	return somethingchanged;
}

AbstractNode *FileModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	assert(evalctx == NULL);
	
	delete this->context;
	this->context = new FileContext(*this, ctx);
	AbstractNode *node = new RootNode(inst);

	try {
		context->initializeModule(*this);

	// FIXME: Set document path to the path of the module
		std::vector<AbstractNode *> instantiatednodes = this->scope.instantiateChildren(context);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
	catch (EvaluationException &e) {
		PRINT(e.what());
	}

	return node;
}

ValuePtr FileModule::lookup_variable(const std::string &name) const
{
	if (!this->context) return ValuePtr::undefined;
	return this->context->lookup_variable(name, true);
}

void register_builtin_group()
{
	Builtins::init("group", new GroupModule());
}
