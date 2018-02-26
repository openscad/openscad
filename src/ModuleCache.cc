#include "ModuleCache.h"
#include "StatCache.h"
#include "FileModule.h"
#include "printutils.h"
#include "openscad.h"

#include <boost/format.hpp>

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <time.h>
#include <sys/stat.h>
#include <algorithm>

/*!
	FIXME: Implement an LRU scheme to avoid having an ever-growing module cache
*/

ModuleCache *ModuleCache::inst = nullptr;

/*!
	Reevaluate the given file and all its dependencies and recompile anything
	needing reevaluation. Updates the cache if necessary.
	The given filename must be absolute.

	Sets the given module reference to the new module, or nullptr on any error (e.g. compile
	error or file not found).

	Returns the latest modification time of the module, its dependencies or includes.
*/
time_t ModuleCache::evaluate(const std::string &filename, FileModule *&module)
{
	module = nullptr;
	auto entry = this->entries.find(filename);
	bool found{entry != this->entries.end()};
	FileModule *lib_mod{found ? entry->second.module : nullptr};
  
	// Don't try to recursively evaluate - if the file changes
	// during evaluation, that would be really bad.
	if (lib_mod && lib_mod->isHandlingDependencies()) return 0;

	// Create cache ID
	struct stat st;
	bool valid = (StatCache::stat(filename.c_str(), st) == 0);

	// If file isn't there, just return and let the cache retain the old module
	if (!valid) return 0;

	// If the file is present, we'll always cache some result
	std::string cache_id = str(boost::format("%x.%x") % st.st_mtime % st.st_size);

	cache_entry &cacheEntry = this->entries[filename];
	// Initialize entry, if new
	if (!found) {
		cacheEntry.module = nullptr;
		cacheEntry.parsed_module = nullptr;
		cacheEntry.cache_id = cache_id;
		cacheEntry.includes_mtime = st.st_mtime;
	}
	cacheEntry.mtime = st.st_mtime;
  
	bool shouldCompile = true;
	if (found) {
		// Files should only be recompiled if the cache ID changed
		if (cacheEntry.cache_id == cache_id) {
			shouldCompile = false;
			// Recompile if includes changed
			if (cacheEntry.parsed_module) {
				time_t mtime = cacheEntry.parsed_module->includesChanged();
				if (mtime > cacheEntry.includes_mtime) {
					cacheEntry.includes_mtime = mtime;
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
		
		delete cacheEntry.parsed_module;
		lib_mod = parse(cacheEntry.parsed_module, textbuf.str().c_str(), filename, false) ? cacheEntry.parsed_module : nullptr;
		PRINTDB("  compiled module: %p", lib_mod);
		cacheEntry.module = lib_mod;
		cacheEntry.cache_id = cache_id;
		
		print_messages_pop();
	}
	
	module = lib_mod;
	// FIXME: Do we need to handle include-only cases?
	time_t deps_mtime = lib_mod ? lib_mod->handleDependencies() : 0;

	return std::max({deps_mtime, cacheEntry.mtime, cacheEntry.includes_mtime});
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
