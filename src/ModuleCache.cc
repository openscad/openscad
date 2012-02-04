#include "ModuleCache.h"
#include "module.h"
#include "printutils.h"
#include "boosty.h"
#include "openscad.h"

#include <stdio.h>
#include <sstream>
#include <sys/stat.h>

ModuleCache *ModuleCache::inst = NULL;

Module *ModuleCache::evaluate(const std::string &filename)
{
	Module *cached = NULL;
	
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	stat(filename.c_str(), &st);

	std::stringstream idstream;
	idstream << std::hex << st.st_mtime << "." << st.st_size;
	std::string cache_id = idstream.str();

	if (this->entries.find(filename) != this->entries.end() && 
			this->entries[filename].cache_id == cache_id) {
#ifdef DEBUG
		PRINTB("Using cached library: %s (%s)", filename % cache_id);
#endif
		PRINTB("%s", this->entries[filename].msg);
		cached = &(*this->entries[filename].module);
	}
	
	if (cached) {
		cached->handleDependencies();
		return cached;
	}
	else {
		if (this->entries.find(filename) != this->entries.end()) {
			PRINTB("Recompiling cached library: %s (%s)", filename % cache_id);
		}
		else {
			PRINTB("Compiling library '%s'.", filename);
		}
	}

	FILE *fp = fopen(filename.c_str(), "rt");
	if (!fp) {
		fprintf(stderr, "WARNING: Can't open library file '%s'\n", filename.c_str());
		return NULL;
	}
	std::stringstream text;
	char buffer[513];
	int ret;
	while ((ret = fread(buffer, 1, 512, fp)) > 0) {
		buffer[ret] = 0;
		text << buffer;
	}
	fclose(fp);

	print_messages_push();

	cache_entry e = { NULL, cache_id, std::string("WARNING: Library `") + filename + "' tries to recursively use itself!" };
	if (this->entries.find(filename) != this->entries.end())
		delete this->entries[filename].module;
	this->entries[filename] = e;

	std::string pathname = boosty::stringy(fs::path(filename).parent_path());
	Module *lib_mod = dynamic_cast<Module*>(parse(text.str().c_str(), pathname.c_str(), 0));

	if (lib_mod) {
		this->entries[filename].module = lib_mod;
		this->entries[filename].msg = print_messages_stack.back();
	} else {
		this->entries.erase(filename);
	}

	print_messages_pop();

	return lib_mod;
}

void ModuleCache::clear()
{
	this->entries.clear();
}

