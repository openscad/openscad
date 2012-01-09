#include "parsersettings.h"
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

std::string librarydir;

void parser_init(const std::string &applicationpath)
{
	path libdir(applicationpath);
	path tmpdir;
#ifdef Q_WS_MAC
	libdir /= "../Resources"; // Libraries can be bundled
	if (!is_directory(libdir / "libraries")) libdir /= "../../..";
#elif defined(Q_OS_UNIX)
	if (is_directory(tmpdir = libdir / "../share/openscad/libraries")) {
		librarydir = tmpdir.generic_string();
	} else if (is_directory(tmpdir = libdir / "../../share/openscad/libraries")) {
		librarydir = tmpdir.generic_string();
	} else if (is_directory(tmpdir = libdir / "../../libraries")) {
		librarydir = tmpdir.generic_string();
	} else
#endif
			if (is_directory(tmpdir = libdir / "libraries")) {
			librarydir = tmpdir.generic_string();
		}
}
