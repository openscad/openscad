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

#include "SourceFile.h"
#include "SourceFileCache.h"
#include "node.h"
#include "printutils.h"
#include "exceptions.h"
#include "modcontext.h"
#include "parsersettings.h"
#include "StatCache.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "boost-utils.h"
namespace fs = boost::filesystem;
#include "FontCache.h"
#include <sys/stat.h>

SourceFile::SourceFile(const std::string &path, const std::string &filename)
	: ASTNode(Location::NONE), is_handling_dependencies(false), path(path), filename(filename)
{
}

SourceFile::~SourceFile()
{
}

void SourceFile::print(std::ostream &stream, const std::string &indent) const
{
	scope.print(stream, indent);
}

void SourceFile::registerUse(const std::string path, const Location &loc)
{
	PRINTDB("registerUse(): (%p) %d, %d - %d, %d (%s) -> %s", this %
			loc.firstLine() % loc.firstColumn() %
			loc.lastLine() % loc.lastColumn() %
			loc.fileName() %
			path);

	auto ext = fs::path(path).extension().generic_string();

	if (boost::iequals(ext, ".otf") || boost::iequals(ext, ".ttf")) {
		if (fs::is_regular(path)) {
			FontCache::instance()->register_font_file(path);
		} else {
			LOG(message_group::Error,Location::NONE,"","Can't read font with path '%1$s'",path);
		}
	} else {
		auto pos = std::find(usedlibs.begin(), usedlibs.end(), path);
		if(pos != usedlibs.end())
			usedlibs.erase(pos);
		usedlibs.insert(usedlibs.begin(), path);
		if (!loc.isNone()) {
			indicatorData.emplace_back(loc.firstLine(), loc.firstColumn(), loc.lastColumn() - loc.firstColumn(), path);
		}
	}
}

void SourceFile::registerInclude(const std::string &localpath, const std::string &fullpath, const Location &loc)
{
	PRINTDB("registerInclude(): (%p) %d, %d - %d, %d (%s) -> %s", this %
			loc.firstLine() % loc.firstColumn() %
			loc.lastLine() % loc.lastColumn() %
			localpath %
			fullpath);

	this->includes[localpath] = fullpath;
	if (!loc.isNone()) {
		indicatorData.emplace_back(loc.firstLine(), loc.firstColumn(), loc.lastColumn() - loc.firstColumn(), fullpath);
	}
}

time_t SourceFile::includesChanged() const
{
	time_t latest = 0;
	for (const auto &item : this->includes) {
		auto mtime = include_modified(item.second);
		if (mtime > latest) latest = mtime;
	}
	return latest;
}

time_t SourceFile::include_modified(const std::string &filename) const
{
	struct stat st;

	if (StatCache::stat(filename.c_str(), st) == 0) {
		return st.st_mtime;
	}

	return 0;
}

/*!
	Check if any dependencies have been modified and recompile them.
	Returns true if anything was recompiled.
*/
time_t SourceFile::handleDependencies(bool is_root)
{
	if(is_root)
		SourceFileCache::clear_markers();
	else
		if (this->is_handling_dependencies) return 0;
	this->is_handling_dependencies = true;

	std::vector<std::pair<std::string,std::string>> updates;

	// If a lib in usedlibs was previously missing, we need to relocate it
	// by searching the applicable paths. We can identify a previously missing module
	// as it will have a relative path.
	time_t latest = 0;
	for (auto filename : this->usedlibs) {

		auto found = true;

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
			auto oldmodule = SourceFileCache::instance()->lookup(filename);
			SourceFile *newmodule;
			auto mtime = SourceFileCache::instance()->evaluate(this->getFullpath(),filename, newmodule);
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
		auto pos = std::find(usedlibs.begin(), usedlibs.end(), files.first);
		if(pos != usedlibs.end())
			*pos = files.second;
	}
	return latest;
}

AbstractNode *SourceFile::instantiate(const std::shared_ptr<const Context>& context, std::shared_ptr<const FileContext>* resulting_file_context) const
{
	auto node = new RootNode();
	try {
		ContextHandle<FileContext> file_context{Context::create<FileContext>(context, this)};
		*resulting_file_context = *file_context;
		this->scope.instantiateModules(*file_context, node);
	} catch (HardWarningException &e) {
		throw;
	} catch (EvaluationException &e) {
		// LOG(message_group::None,Location::NONE,"",e.what()); //please output the message before throwing the exception
		*resulting_file_context = nullptr;
	}
	return node;
}

//please preferably use getFilename
//if you compare filenames (which is the origin of this method),
//please call getFilename first and use this method only as a fallback
const std::string SourceFile::getFullpath() const {
	if(fs::path(this->filename).is_absolute()){
		return this->filename;
	}else if(!this->path.empty()){
		return (fs::path(this->path) / fs::path(this->filename)).generic_string();
	}else{
		return "";
	}
}
