#include "handle_dep.h"
#include <string>
#include <sstream>
#include <stdlib.h> // for system()
#include <boost/unordered_set.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
using namespace boost::filesystem;
#include "boosty.h"
#include <Magick++.h>


boost::unordered_set<std::string> dependencies;
const char *make_command = NULL;

void handle_dep(const std::string &filename)
{
	path filepath(filename);
	if ( boosty::is_absolute( filepath )) {
		dependencies.insert(filename);
	}
	else {
		dependencies.insert((current_path() / filepath).string());
	}
	if (!exists(filepath) && make_command) {
		std::stringstream buf;
		buf << make_command << " '" << boost::regex_replace(filename, boost::regex("'"), "'\\''") << "'";
		system(buf.str().c_str()); // FIXME: Handle error
	}
}

bool write_deps(const std::string &filename, const std::string &output_file)
{
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
}

bool is_image(const std::string &filename) {
    Magick::Image image;
    bool isImage;
    try {
        image.read( filename );
        isImage=true;
    } catch( Magick::Exception &error_ ) {
        isImage=false;
    }
    return isImage;
}

bool is_stl(const std::string &filename) {
    boost::regex expression(".*\\.([Ss][Tt][Ll])$");
    // path file_ext = path(filename).extension();
    if( boost::regex_match(filename, expression) ) {
        return true;
    } else {
        return false;
    }
}

bool is_dxf(const std::string &filename) {
    boost::regex expression(".*\\.([Dd][Xx][Ff])$");
    // path file_ext = path(filename).extension();
    if( boost::regex_match(filename, expression) ) {
        return true;
    } else {
        return false;
    }
}
