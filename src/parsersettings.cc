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
fs::path search_libs(const std::string &filename)
{
	BOOST_FOREACH(const std::string &dir, librarypath) {
		fs::path usepath = fs::path(dir) / filename;
		if (fs::exists(usepath) && !fs::is_directory(usepath)) {
			PRINTB("found %s in %s", filename % dir );
			return usepath.string();
		}
	}
	return fs::path();
}

// files must be 'ordinary' - they mus exist and be non-directories
bool check_valid( fs::path p, std::vector<std::string> openfilenames )
{
	if (p.empty()) {
		PRINTB("WARNING: %s invalid - file path is blank",p);
		return false;
	}
	if (!p.has_parent_path()) {
		PRINTB("WARNING: %s invalid - no parent path",p);
		return false;
	}
	if (!fs::exists(p)) {
		PRINTB("WARNING: %s invalid - file not found",p);
		// searched ===
		return false;
	}
	if (fs::is_directory(p)) {
		PRINTB("WARNING: %s invalid - points to a directory",p);
		return false;
	}
	std::string fullname = boosty::stringy( p );
	BOOST_FOREACH(std::string &s, openfilenames) {
		PRINTB("WARNING: circular include with %s", fullname);
		if (s == fullname) return false;
	}
	return true;
}

// check if file is valid, search path for valid simple file
// return empty path on failure
fs::path find_valid_path( fs::path sourcepath, std::string filename,
	std::vector<std::string> openfilenames )
{
	fs::path fpath = fs::path( filename );

	if ( boosty::is_absolute( fpath ) )
		if ( check_valid( fpath, openfilenames ) )
			return boosty::absolute( fpath );


	fpath = sourcepath / filename;
	if ( check_valid( fpath, openfilenames ) ) return fpath;
	fpath = search_libs( filename );
	if ( check_valid( fpath, openfilenames ) ) return fpath;
	return fs::path();
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
