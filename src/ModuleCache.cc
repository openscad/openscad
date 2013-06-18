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
	Reevaluate the given file and recompile if necessary.
	Returns NULL on any error (e.g. compile error or file not found)

	If the given filename is relative, it means that the module hasn't been
	previously located.
*/
FileModule *ModuleCache::evaluate(const std::string &filename)
{
	FileModule *lib_mod = (this->entries.find(filename) != this->entries.end()) ?
		&(*this->entries[filename].module) : NULL;

	// Don't try to recursively evaluate - if the file changes
	// during evaluation, that would be really bad.
	if (lib_mod && lib_mod->isHandlingDependencies()) return lib_mod;

	bool shouldCompile = true;

	// Create cache ID
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	bool valid = (stat(filename.c_str(), &st) == 0);

  // If file isn't there, just return and let the cache retain the old module
	if (!valid) return NULL;
	
	std::string cache_id = str(boost::format("%x.%x") % st.st_mtime % st.st_size);

	// Lookup in cache
	if (lib_mod) {
		if (this->entries[filename].cache_id == cache_id) {
			shouldCompile = false;
			if (lib_mod->includesChanged()) {
				lib_mod = NULL;
				shouldCompile = true;
			}
		}
	}
	else {
		shouldCompile = valid;
	}

	// If cache lookup failed (non-existing or old timestamp), compile module
	if (shouldCompile) {
#ifdef DEBUG
		if (this->entries.find(filename) != this->entries.end()) {
			PRINTB("Recompiling cached library: %s (%s)", filename % cache_id);
		}
		else {
			PRINTB("Compiling library '%s'.", filename);
		}
#endif

		std::ifstream ifs(filename.c_str());
		if (!ifs.is_open()) {
			PRINTB("WARNING: Can't open library file '%s'\n", filename);
			return NULL;
		}
		std::stringstream textbuf;
		textbuf << ifs.rdbuf();
		textbuf << "\n" << commandline_commands;

		print_messages_push();

		FileModule *oldmodule = NULL;
		cache_entry e = { NULL, cache_id };
		if (this->entries.find(filename) != this->entries.end()) {
			oldmodule = this->entries[filename].module;
		}
		this->entries[filename] = e;
		
		std::string pathname = boosty::stringy(fs::path(filename).parent_path());
		lib_mod = dynamic_cast<FileModule*>(parse(textbuf.str().c_str(), pathname.c_str(), false));
		PRINTB_NOCACHE("  compiled module: %p", lib_mod);
		
		if (lib_mod) {
			// We defer deletion so we can ensure that the new module won't
			// have the same address as the old
			delete oldmodule;
			this->entries[filename].module = lib_mod;
		} else {
			this->entries.erase(filename);
		}
		
		print_messages_pop();
	}

	if (lib_mod) {
		lib_mod->handleDependencies();
	}

	return lib_mod;
}

void ModuleCache::clear()
{
	this->entries.clear();
}

FileModule *ModuleCache::lookup(const std::string &filename)
{
	return (this->entries.find(filename) != this->entries.end()) ?
		&(*this->entries[filename].module) : NULL;
}

