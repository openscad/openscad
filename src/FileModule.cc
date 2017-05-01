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

#include "FileModule.h"
#include "ModuleCache.h"
#include "node.h"
#include "printutils.h"
#include "exceptions.h"
#include "modcontext.h"
#include "parsersettings.h"
#include "StatCache.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "FontCache.h"
#include <sys/stat.h>

FileModule::~FileModule()
{
}

std::string FileModule::dump(const std::string &indent, const std::string &name) const
{
	return scope.dump(indent);
}

void FileModule::registerUse(const std::string path)
{
	auto extraw = fs::path(path).extension().generic_string();
	auto ext = boost::algorithm::to_lower_copy(extraw);
	
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

void FileModule::registerInclude(const std::string &localpath, const std::string &fullpath)
{
	this->includes[localpath] = {fullpath};
}

time_t FileModule::includesChanged() const
{
	time_t latest = 0;
	for (const auto &item : this->includes) {
		auto mtime = include_modified(item.second);
		if (mtime > latest) latest = mtime;
	}
	return latest;
}

time_t FileModule::include_modified(const IncludeFile &inc) const
{
	struct stat st;

	if (StatCache::stat(inc.filename.c_str(), &st) == 0) {
		return st.st_mtime;
	}
	
	return 0;
}

/*!
	Check if any dependencies have been modified and recompile them.
	Returns true if anything was recompiled.
*/
time_t FileModule::handleDependencies()
{
	if (this->is_handling_dependencies) return 0;
	this->is_handling_dependencies = true;

	std::vector<std::pair<std::string,std::string>> updates;

	// If a lib in usedlibs was previously missing, we need to relocate it
	// by searching the applicable paths. We can identify a previously missing module
	// as it will have a relative path.
	time_t latest = 0;
	for (auto filename : this->usedlibs) {

		auto wasmissing = false;
		auto found = true;

		// Get an absolute filename for the module
		if (!fs::path(filename).is_absolute()) {
			wasmissing = true;
			auto fullpath = find_valid_path(this->path, filename);
			if (!fullpath.empty()) {
				updates.emplace_back(filename, fullpath.generic_string());
				filename = fullpath.generic_string();
			}
			else {
				found = false;
			}
		}

		if (found) {
			auto wascached = ModuleCache::instance()->isCached(filename);
			auto oldmodule = ModuleCache::instance()->lookup(filename);
			FileModule *newmodule;
			auto mtime = ModuleCache::instance()->evaluate(filename, newmodule);
			if (mtime > latest) latest = mtime;
			auto changed = newmodule && newmodule != oldmodule;
			// Detect appearance but not removal of files, and keep old module
			// on compile errors (FIXME: Is this correct behavior?)
			if (changed) {
				PRINTDB("  %s: %p -> %p", filename % oldmodule % newmodule);
			}
			else {
				PRINTDB("  %s: %p", filename % oldmodule);
			}
			// Only print warning if we're not part of an automatic reload
			if (!newmodule && !wascached && !wasmissing) {
				PRINTB_NOCACHE("WARNING: Failed to compile library '%s'.", filename);
			}
		}
	}

	// Relative filenames which were located are reinserted as absolute filenames
	typedef std::pair<std::string,std::string> stringpair;
	for (const auto &files : updates) {
		this->usedlibs.erase(files.first);
		this->usedlibs.insert(files.second);
	}
	this->is_handling_dependencies = false;
	return latest;
}

AbstractNode *FileModule::instantiate(const Context *ctx, const ModuleInstantiation *inst,
																			EvalContext *evalctx) const
{
	assert(evalctx == nullptr);
	
	FileContext context(ctx);
	return this->instantiateWithFileContext(&context, inst, evalctx);
}

AbstractNode *FileModule::instantiateWithFileContext(FileContext *ctx, const ModuleInstantiation *inst,
																										 EvalContext *evalctx) const
{
	assert(evalctx == nullptr);
	
	auto node = new RootNode(inst);
	try {
		ctx->initializeModule(*this); // May throw an ExperimentalFeatureException
		// FIXME: Set document path to the path of the module
		auto instantiatednodes = this->scope.instantiateChildren(ctx);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
	catch (EvaluationException &e) {
		PRINT(e.what());
	}

	return node;
}
