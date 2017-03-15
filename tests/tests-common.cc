#include "tests-common.h"
#include "openscad.h"
#include "FileModule.h"
#include "handle_dep.h"

#include <boost/filesystem.hpp>

#include <sstream>
#include <fstream>

namespace fs=boost::filesystem;

/*!
	fakepath is used to force the parser to believe that the file is
	read from this location, in order to ensure that filepaths are
	eavluated relative to this path (for testing purposes).
*/
FileModule *parsefile(const char *filename, const char *fakepath)
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
		std::string pathname;
		if (fakepath) pathname = fakepath;
		else pathname = fs::path(filename).parent_path().generic_string();
		if(!parse(root_module, text.c_str(), pathname.c_str(), false)) {
			delete root_module;             // parse failed
			root_module = NULL;
		}
		if (root_module) {
			root_module->handleDependencies();
		}
	}
	return root_module;
}
