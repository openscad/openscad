#include "parsersettings.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "boosty.h"
#include <boost/algorithm/string.hpp>
#ifdef __APPLE__
#include "CocoaUtils.h"
#endif

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
  // Add paths from OPENSCADPATH before adding built-in paths
	const char *openscadpaths = getenv("OPENSCADPATH");
	if (openscadpaths) {
		std::string paths(openscadpaths);
    typedef boost::split_iterator<std::string::iterator> string_split_iterator;
    for (string_split_iterator it =
					 make_split_iterator(paths, first_finder(":", boost::is_iequal()));
				 it != string_split_iterator();
				 ++it) {
		add_librarydir(boosty::absolute(fs::path(boost::copy_range<std::string>(*it))).string());
    }
	}

	// FIXME: Add ~/.openscad/libraries
#if defined(__APPLE__) && !defined(OPENSCAD_TESTING)
	fs::path docdir(CocoaUtils::documentsPath());
	add_librarydir(boosty::stringy(docdir / "OpenSCAD" / "libraries"));
#endif

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
