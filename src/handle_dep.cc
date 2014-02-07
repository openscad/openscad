#include "handle_dep.h"
#include <string>
#include <sstream>
#include <stdlib.h> // for system()
#include <boost/unordered_set.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "boosty.h"
#include "printutils.h"
#include "PlatformUtils.h"

boost::unordered_set<std::string> dependencies;
const char *make_command = NULL;

void handle_dep(const std::string &filename)
{
	fs::path filepath(filename);
	if ( boosty::is_absolute( filepath )) {
		dependencies.insert(filename);
	}
	else {
		dependencies.insert((fs::current_path() / filepath).string());
	}
	if (!fs::exists(filepath) && make_command) {
		std::stringstream buf;
		buf << make_command << " '" << boost::regex_replace(filename, boost::regex("'"), "'\\''") << "'";
		int status = system(buf.str().c_str());
		//PRINTDB("handle_dep system() status: %i",status);
		if (status==-1) {
			PRINTB("ERROR: handle_dep failed for %s",filename);
			PRINTB("ERROR: make_command was: %s",make_command);
		} // FIXME - handle error better
	}
}

/* write dependencies for the given filename (stored in a global 
variable) to the given output file. return false on failure, true on 
success. */
bool write_deps(const std::string &filename, const std::string &output_file)
{
	// TESTME - does it work on Windows to write non-binary file mode?
	// TESTME - does it work on Mac/Unix to write binary file mode?
	PlatformUtils::ofstream ofs( filename.c_str() );
	if (!ofs.good()) {
		fprintf(stderr, "Can't open dependencies file `%s' for writing!\n", filename.c_str());
		return false;
	}
	ofs << output_file;
	BOOST_FOREACH(const std::string &str, dependencies) {
		ofs << " \\\n\t" << str;
	}
	ofs << "\n";
	ofs.close();
	return true;
/*
	FILE *fp = fopen(filename.c_str(), "wt");
	if (!fp) {
		fprintf(stderr, "Can't open dependencies file `%s' for writing!\n", filename.c_str());
		return false;
	}
	fprintf(fp, "%s:", output_file.c_str());

	BOOST_FOREACH(const std::string &str, dependencies) {
		fprintf(fp, " \\\n\t%s", str.c_str());
	}
	fprintf(fp, "\n");
	fclose(fp);
	return true;
*/
}
