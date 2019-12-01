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
#include "openscad.h"
#include "evalcontext.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "boost-utils.h"
namespace fs = boost::filesystem;
#include "FontCache.h"
#include <sys/stat.h>

FileModule::FileModule(const std::string &path, const std::string &filename)
	: ASTNode(Location::NONE), is_handling_dependencies(false), path(path), filename(filename)
{
}

FileModule::~FileModule()
{
}

void FileModule::print(std::ostream &stream, const std::string &indent) const
{
	for (const auto &node : this->externalList) {
		node->print(stream, indent);
	}
	scope.print(stream, indent);
}

void FileModule::addExternalNode(const std::shared_ptr<ExternalNode> &node)
{
	this->externalList.push_back(node);
 	const auto &loc = node->location();
 	if (!loc.isNone()) {
 		this->indicatorData.emplace_back(loc.firstLine(), loc.firstColumn(), loc.lastColumn() - loc.firstColumn(), node->filename);
 	}
}

void FileModule::resolveUseNodes()
{
	for (const auto &node : this->externalList) {
		const auto usenode = dynamic_pointer_cast<const UseNode>(node);
		if (!usenode) continue;
		const auto &filename = usenode->filename;
		const auto fullpath = find_valid_path(fs::path(this->path), fs::path(filename));
    // handle_dep(fullpath.generic_string());
		const auto extraw = fs::path(filename).extension().generic_string();
		const auto ext = boost::algorithm::to_lower_copy(extraw);
		
		if ((ext == ".otf") || (ext == ".ttf")) {
			if (fs::is_regular(filename)) {
				FontCache::instance()->register_font_file(filename);
			} else {
				PRINTB("ERROR: Can't read font with path '%s'", filename);
			}
		} else {
			externalDict.emplace(filename, node);
		}
	}
}

void FileModule::resolveIncludeNodes()
{
	for (const auto &node : this->externalList) {
		const auto includenode = dynamic_pointer_cast<const IncludeNode>(node);
		if (!includenode) continue;
		const fs::path localpath{includenode->filename};
		const auto fullpath = find_valid_path(this->path, localpath);
		if (fullpath.empty()) {
			PRINTB("WARNING: Can't open include file '%s'.", localpath.generic_string());
			continue;
		};

		const auto fullname = fullpath.generic_string();
    // handle_dep(fullname);
		// FIXME: Instead of the below, try to use ModuleCache to access both include nodes and use nodes
		// o parse fullname into FileModule/IncludeModule
		std::ifstream ifs{fullname};
		if (!ifs.is_open()) {
			PRINTB("Can't open include file '%s'!\n", fullname.c_str());
			return;
		}
		std::string text{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
		FileModule *inc_mod{nullptr};
		// FIXME: Does this->getFullpath() constitute the main file?
		if (!parse(inc_mod, text, fullname, this->getFullpath(), false)) {
			PRINTB("Can't parse include file '%s'!\n", fullname.c_str());
			return;
		}
		// Add inc_mod to a member container
	}
}

time_t FileModule::includesChanged() const
{
	time_t latest = 0;
	for (const auto &item : this->externalList) {
		if (auto includeNode = dynamic_cast<IncludeNode*>(item.get())) {
			auto mtime = includeModified(*includeNode);
			if (mtime > latest) latest = mtime;
		}
	}
	return latest;
}

time_t FileModule::includeModified(const IncludeNode &node) const
{
	struct stat st;

	if (StatCache::stat(node.filename, st) == 0) {
		return st.st_mtime;
	}
	
	return 0;
}

/*!
	Check if any dependencies have been modified and recompile them.
	Returns true if anything was recompiled.
*/
// FIXME: Do we need a mode for include-only?
time_t FileModule::handleDependencies(bool is_root)
{
	if(is_root)
		ModuleCache::clear_markers();
	else
		if (this->is_handling_dependencies) return 0;
	this->is_handling_dependencies = true;

	std::vector<std::pair<std::string,std::string>> updates;

	// If a lib in externalDict was previously missing, we need to relocate it
	// by searching the applicable paths. We can identify a previously missing module
	// as it will have a relative path.
	time_t latest = 0;
	for (const auto &externalEntry : this->externalDict) {
		std::string filename = externalEntry.first;
		bool found = true;

		// Get an absolute filename for the module
		if (!fs::path(filename).is_absolute()) {
			auto fullpath = find_valid_path(this->path, filename);
			if (!fullpath.empty()) {
				auto newfilename = fullpath.generic_string();
				updates.emplace_back(filename, newfilename);
				filename = newfilename;
			}
			else {
				found = false;
			}
		}

		if (found) {
			auto oldmodule = ModuleCache::instance()->lookup(filename);
			FileModule *newmodule;
			auto mtime = ModuleCache::instance()->evaluate(this->getFullpath(),filename, newmodule);
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
		}
	}

	// Relative filenames which were located are reinserted as absolute filenames
	for (const auto &files : updates) {
		this->externalDict.insert({files.second, this->externalDict.at(files.first)});
		this->externalDict.erase(files.first);
	}
	return latest;
}

AbstractNode *FileModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	assert(!evalctx);
	
	ContextHandle<FileContext> context{Context::create<FileContext>(ctx)};
	return this->instantiateWithFileContext(context.ctx, inst, evalctx);
}

AbstractNode *FileModule::instantiateWithFileContext(const std::shared_ptr<FileContext>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	assert(!evalctx);
	
	auto node = new RootNode(inst);
	try {
		ctx->initializeModule(*this); // May throw an ExperimentalFeatureException
		// FIXME: Set document path to the path of the module
		auto instantiatednodes = this->scope.instantiateChildren(ctx);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	} catch (EvaluationException &e) {
		//PRINT(e.what()); //please output the message before throwing the exception
	}

	return node;
}

std::vector<shared_ptr<UseNode>> FileModule::getUseNodes() const
{
	std::vector<shared_ptr<UseNode>> useNodes;
	for (const auto &node : this->externalList) {
		if (auto useNode = dynamic_pointer_cast<UseNode>(node)) {
			useNodes.emplace_back(useNode);
		}
	}
	return useNodes;
}

void FileModule::resolveExternals()
{
	// FIXME: Manage return values from these two functions?
	this->resolveIncludeNodes();
	this->resolveUseNodes();
}

//please preferably use getFilename
//if you compare filenames (which is the origin of this method),
//please call getFilename first and use this method only as a fallback
const std::string FileModule::getFullpath() const {
	if(fs::path(this->filename).is_absolute()){
		return this->filename;
	}else if(!this->path.empty()){
		return (fs::path(this->path) / fs::path(this->filename)).generic_string();
	}else{
		return "";
	}
}

void FileModule::resolveAssignments()
{
	// FIXME: We need to walk the tree and call resolveAssignments() for every LocalScope
	// Note: Needs to happen _after_ resolving included files
	this->scope.resolveAssignments();
}
