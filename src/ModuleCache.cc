#include "ModuleCache.h"
#include "module.h"
#include "printutils.h"
#include "openscad.h"

#include "boosty.h"
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

/*!
	FIXME: Implement an LRU scheme to avoid having an ever-growing module cache
*/

ModuleCache *ModuleCache::inst = NULL;

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
		PRINTB("Using cached library: %s (%s)", filename % cache_id);
#endif
		PRINTB("%s", this->entries[filename].msg);
		lib_mod = &(*this->entries[filename].module);
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
		
		cache_entry e = { NULL, cache_id, std::string("WARNING: Library `") + filename + "' tries to recursively use itself!" };
		if (this->entries.find(filename) != this->entries.end())
			delete this->entries[filename].module;
		this->entries[filename] = e;
		
		std::string pathname = boosty::stringy(fs::path(filename).parent_path());
		lib_mod = dynamic_cast<Module*>(parse(text.c_str(), pathname.c_str(), 0));
		
		if (lib_mod) {
			this->entries[filename].module = lib_mod;
			this->entries[filename].msg = print_messages_stack.back();
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

