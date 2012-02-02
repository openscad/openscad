#include "parsersettings.h"
#include <boost/filesystem.hpp>
#include <qglobal.h> // Needed for Q_ defines - move the offending code somewhere else

using namespace boost::filesystem;
#include "boosty.h"

std::string librarydir;

void set_librarydir(const std::string &libdir)
{
	librarydir = libdir;
}

const std::string &get_librarydir()
{
	return librarydir;
}

void parser_init(const std::string &applicationpath)
{
	std::string librarydir;
	path libdir(applicationpath);
	path tmpdir;
#ifdef Q_WS_MAC
	libdir /= "../Resources"; // Libraries can be bundled
	if (!is_directory(libdir / "libraries")) libdir /= "../../..";
#elif defined(Q_OS_UNIX)
	if (is_directory(tmpdir = libdir / "../share/openscad/libraries")) {
		librarydir = boosty::stringy( tmpdir );
	} else if (is_directory(tmpdir = libdir / "../../share/openscad/libraries")) {
		librarydir = boosty::stringy( tmpdir );
	} else if (is_directory(tmpdir = libdir / "../../libraries")) {
		librarydir = boosty::stringy( tmpdir );
	} else
#endif
		if (is_directory(tmpdir = libdir / "libraries")) {
			librarydir = boosty::stringy( tmpdir );
		}
	set_librarydir(librarydir);
}
