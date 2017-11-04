#include "ModuleCache.h"
#include "StatCache.h"
#include "FileModule.h"
#include "printutils.h"
#include "openscad.h"

#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <time.h>
#include <sys/stat.h>
#include <algorithm>

namespace fs = boost::filesystem;
//#include "parsersettings.h"
/*!
   FIXME: Implement an LRU scheme to avoid having an ever-growing module cache
 */

ModuleCache *ModuleCache::inst = nullptr;

/*!
   Reevaluate the given file and all it's dependencies and recompile anything
   needing reevaluation. Updates the cache if necessary.
   The given filename must be absolute.

   Sets the module reference to the new module, or nullptr on any error (e.g. compile
   error or file not found).

   Returns the latest mod time of the modul or its dependencies or includes.
 */
time_t ModuleCache::evaluate(const std::string &filename, FileModule * &module)
{
	module = nullptr;
	FileModule *lib_mod = nullptr;
	bool found = false;
	if (this->entries.find(filename) != this->entries.end()) {
		found = true;
		lib_mod = this->entries[filename].module;
	}

	// Don't try to recursively evaluate - if the file changes
	// during evaluation, that would be really bad.
	if (lib_mod && lib_mod->isHandlingDependencies()) return 0;

	// Create cache ID
	struct stat st;
	bool valid = (StatCache::stat(filename.c_str(), &st) == 0);

	// If file isn't there, just return and let the cache retain the old module
	if (!valid) return 0;

	// If the file is present, we'll always cache some result
	std::string cache_id = str(boost::format("%x.%x") % st.st_mtime % st.st_size);

	cache_entry &entry = this->entries[filename];
	// Initialize entry, if new
	if (!found) {
		entry.module = nullptr;
		entry.parsed_module = nullptr;
		entry.cache_id = cache_id;
		entry.includes_mtime = st.st_mtime;
	}
	entry.mtime = st.st_mtime;

	bool shouldCompile = true;
	if (found) {
		// Files should only be recompiled if the cache ID changed
		if (entry.cache_id == cache_id) {
			shouldCompile = false;
			// Recompile if includes changed
			if (entry.parsed_module) {
				time_t mtime = entry.parsed_module->includesChanged();
				if (mtime > entry.includes_mtime) {
					entry.includes_mtime = mtime;
					shouldCompile = true;
				}
			}
		}
	}

#ifdef DEBUG
	// Causes too much debug output
	//if (!shouldCompile) PRINTB("Using cached library: %s (%p)", filename % lib_mod);
#endif

	// If cache lookup failed (non-existing or old timestamp), compile module
	if (shouldCompile) {
#ifdef DEBUG
		if (found) {
			PRINTDB("Recompiling cached library: %s (%s)", filename % cache_id);
		}
		else {
			PRINTDB("Compiling library '%s'.", filename);
		}
#endif

		std::stringstream textbuf;
		{
			std::ifstream ifs(filename.c_str());
			if (!ifs.is_open()) {
				PRINTB("WARNING: Can't open library file '%s'\n", filename);
				return 0;
			}
			textbuf << ifs.rdbuf();
		}
		textbuf << "\n" << commandline_commands;

		print_messages_push();

		fs::path pathname = fs::path(filename);
		delete entry.parsed_module;
		lib_mod = parse(entry.parsed_module, textbuf.str().c_str(), pathname, false) ? entry.parsed_module : nullptr;
		PRINTDB("  compiled module: %p", lib_mod);
		entry.module = lib_mod;
		entry.cache_id = cache_id;

		print_messages_pop();
	}

	module = lib_mod;
	time_t deps_mtime = lib_mod ? lib_mod->handleDependencies() : 0;

	return std::max(deps_mtime, std::max(entry.mtime, entry.includes_mtime));
}

void ModuleCache::clear()
{
	this->entries.clear();
}

FileModule *ModuleCache::lookup(const std::string &filename)
{
	return isCached(filename) ? this->entries[filename].module : nullptr;
}

bool ModuleCache::isCached(const std::string &filename)
{
	return this->entries.find(filename) != this->entries.end();
}
