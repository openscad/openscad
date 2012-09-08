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

/*!
	FIXME: Implement an LRU scheme to avoid having an ever-growing module cache
*/

ModuleCache *ModuleCache::inst = NULL;

static bool is_modified(const std::string &filename, const time_t &mtime)
{
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	stat(filename.c_str(), &st);
	return (st.st_mtime > mtime);
}

Module *ModuleCache::evaluate(const std::string &filename)
{
	Module *lib_mod = NULL;

  // Create cache ID
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	stat(filename.c_str(), &st);

	std::string cache_id = str(boost::format("%x.%x") % st.st_mtime % st.st_size);

  // Lookup in cache
	if (this->entries.find(filename) != this->entries.end() && 
			this->entries[filename].cache_id == cache_id) {
#ifdef DEBUG
// Causes too much debug output
//		PRINTB("Using cached library: %s (%s)", filename % cache_id);
#endif
		lib_mod = &(*this->entries[filename].module);

		BOOST_FOREACH(const Module::IncludeContainer::value_type &item, lib_mod->includes) {
			if (is_modified(item.first, item.second)) {
				lib_mod = NULL;
				break;
			}
		}
	}

  // If cache lookup failed (non-existing or old timestamp), compile module
	if (!lib_mod) {
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
		std::string text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		print_messages_push();

		Module *oldmodule = NULL;
		cache_entry e = { NULL, cache_id };
		if (this->entries.find(filename) != this->entries.end()) {
			oldmodule = this->entries[filename].module;
		}
		this->entries[filename] = e;
		
		std::string pathname = boosty::stringy(fs::path(filename).parent_path());
		lib_mod = dynamic_cast<Module*>(parse(text.c_str(), pathname.c_str(), false));
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

	if (lib_mod) lib_mod->handleDependencies();

	return lib_mod;
}

void ModuleCache::clear()
{
	this->entries.clear();
}

