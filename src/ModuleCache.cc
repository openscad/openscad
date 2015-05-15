#include "ModuleCache.h"
#include "module.h"
#include "printutils.h"
#include "openscad.h"

#include "boosty.h"
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <time.h>
#include <sys/stat.h>

//#include "parsersettings.h"
/*!
	FIXME: Implement an LRU scheme to avoid having an ever-growing module cache
*/

ModuleCache *ModuleCache::inst = NULL;

/*!
	Reevaluate the given file and all it's dependencies and recompile anything
	needing reevaluation. Updates the cache if necessary.
	The given filename must be absolute.

	Sets the module reference to the new module, or NULL on any error (e.g. compile
	error or file not found).

	Returns true if anything was compiled (module or dependencies) and false otherwise.
*/
bool ModuleCache::evaluate(const std::string &filename, FileModule *&module)
{
	FileModule *lib_mod = NULL;
	bool found = false;
	if (this->entries.find(filename) != this->entries.end()) {
		found = true;
		lib_mod = this->entries[filename].module;
	}
  
	// Don't try to recursively evaluate - if the file changes
	// during evaluation, that would be really bad.
	if (lib_mod && lib_mod->isHandlingDependencies()) return false;

	// Create cache ID
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	bool valid = (stat(filename.c_str(), &st) == 0);

	// If file isn't there, just return and let the cache retain the old module
	if (!valid) return false;

	// If the file is present, we'll always cache some result
	std::string cache_id = str(boost::format("%x.%x") % st.st_mtime % st.st_size);

	cache_entry &entry = this->entries[filename];
	// Initialize entry, if new
	if (!found) {
		entry.module = NULL;
		entry.cache_id = cache_id;
	}
  
	bool shouldCompile = true;
	if (found) {
		// Files should only be recompiled if the cache ID changed
		if (entry.cache_id == cache_id) {
			shouldCompile = false;
			// Recompile if includes changed
			if (lib_mod && lib_mod->includesChanged()) {
				lib_mod = NULL;
				shouldCompile = true;
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
				return false;
			}
			textbuf << ifs.rdbuf();
		}
		textbuf << "\n" << commandline_commands;
		
		print_messages_push();
		
		FileModule *oldmodule = lib_mod;
		
		std::string pathname = boosty::stringy(fs::path(filename).parent_path());
		lib_mod = dynamic_cast<FileModule*>(parse(textbuf.str().c_str(), pathname.c_str(), false));
		PRINTDB("  compiled module: %p", lib_mod);
		
		// We defer deletion so we can ensure that the new module won't
		// have the same address as the old
		if (oldmodule) delete oldmodule;
		entry.module = lib_mod;
		entry.cache_id = cache_id;
		
		print_messages_pop();
	}
	
	module = lib_mod;
	bool depschanged = lib_mod ? lib_mod->handleDependencies() : false;

	return shouldCompile || depschanged;
}

void ModuleCache::clear()
{
	this->entries.clear();
}

FileModule *ModuleCache::lookup(const std::string &filename)
{
	return isCached(filename) ? this->entries[filename].module : NULL;
}

bool ModuleCache::isCached(const std::string &filename)
{
	return this->entries.find(filename) != this->entries.end();
}

