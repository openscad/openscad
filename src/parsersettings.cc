#include "parsersettings.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "boosty.h"
#include <qglobal.h> // Needed for Q_ defines - move the offending code somewhere else

namespace fs = boost::filesystem;

std::vector<std::string> librarypath;

void add_librarydir(const std::string &libdir)
{
	librarypath.push_back(libdir);
}

/*!
	Searces for the given file in library paths and returns the full path if found.
	Returns an empty path if file cannot be found or filename is a directory.
*/
std::string locate_file(const std::string &filename)
{
	BOOST_FOREACH(const std::string &dir, librarypath) {
		fs::path usepath = fs::path(dir) / filename;
		if (fs::exists(usepath) && !fs::is_directory(usepath)) return usepath.string();
	}
	return std::string();
}

void parser_init(const std::string &applicationpath)
{
  // Add path from OPENSCADPATH before adding built-in paths
	const char *openscadpath = getenv("OPENSCADPATH");
	if (openscadpath) {
		add_librarydir(boosty::absolute(fs::path(openscadpath)).string());
	}

	// FIXME: Support specifying more than one path in OPENSCADPATH
	// FIXME: Add ~/.openscad/libraries
	// FIXME: Add ~/Documents/OpenSCAD/libraries on Mac?

	std::string librarydir;
	fs::path libdir(applicationpath);
	fs::path tmpdir;
#ifdef __APPLE__
	libdir /= "../Resources"; // Libraries can be bundled
	if (!is_directory(libdir / "libraries")) libdir /= "../../..";
#elif !defined(WIN32)
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
	if (!librarydir.empty()) add_librarydir(librarydir);
}
