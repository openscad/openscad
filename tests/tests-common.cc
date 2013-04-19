#include "tests-common.h"
#include "openscad.h"
#include "module.h"
#include "handle_dep.h"
#include "boosty.h"

#include <sstream>
#include <fstream>

FileModule *parsefile(const char *filename)
{
	FileModule *root_module = NULL;

	handle_dep(filename);
	std::ifstream ifs(filename);
	if (!ifs.is_open()) {
		fprintf(stderr, "Can't open input file `%s'!\n", filename);
	}
	else {
		std::string text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		text += "\n" + commandline_commands;
		std::string pathname = boosty::stringy(fs::path(filename).parent_path());
		root_module = parse(text.c_str(), pathname.c_str(), false);
		if (root_module) {
			root_module->handleDependencies();
		}
	}
	return root_module;
}
