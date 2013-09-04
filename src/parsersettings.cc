#include "parsersettings.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "boosty.h"
#include <boost/algorithm/string.hpp>
#include "PlatformUtils.h"

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
fs::path search_libs(const fs::path &localpath)
{
	BOOST_FOREACH(const std::string &dir, librarypath) {
		fs::path usepath = fs::path(dir) / localpath;
		if (fs::exists(usepath) && !fs::is_directory(usepath)) {
			return usepath.string();
		}
	}
	return fs::path();
}

// files must be 'ordinary' - they must exist and be non-directories
// FIXME: We cannot print any output here since these function is called periodically
// from "Automatic reload and compile"
static bool check_valid(const fs::path &p, const std::vector<std::string> *openfilenames)
{
	if (p.empty()) {
		//PRINTB("WARNING: File path is blank: %s",p);
		return false;
	}
	if (!p.has_parent_path()) {
		//PRINTB("WARNING: No parent path: %s",p);
		return false;
	}
	if (!fs::exists(p)) {
		//PRINTB("WARNING: File not found: %s",p);
		return false;
	}
	if (fs::is_directory(p)) {
		//PRINTB("WARNING: %s invalid - points to a directory",p);
		return false;
	}
	std::string fullname = boosty::stringy(p);
  // Detect circular includes
	if (openfilenames) {
		BOOST_FOREACH(const std::string &s, *openfilenames) {
			if (s == fullname) {
//				PRINTB("WARNING: circular include file %s", fullname);
				return false;
			}
		}
	}
	return true;
}

/*!
	Check if the given filename is valid.

	If the given filename is absolute, do a simple check.
	If not, search the applicable paths for a valid file.

	Returns the absolute path to a valid file, or an empty path if no
	valid files could be found.
*/
fs::path find_valid_path(const fs::path &sourcepath, 
												 const fs::path &localpath,
												 const std::vector<std::string> *openfilenames)
{
	if (boosty::is_absolute(localpath)) {
		if (check_valid(localpath, openfilenames)) return boosty::absolute(localpath);
	}
	else {
		fs::path fpath = sourcepath / localpath;
		if (check_valid(fpath, openfilenames)) return fpath;
		fpath = search_libs(localpath);
		if (!fpath.empty() && check_valid(fpath, openfilenames)) return fpath;
	}
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

	// This is the built-in user-writable library path
#ifndef OPENSCAD_TESTING
  // This will resolve to ~/Documents on Mac, "My Documents" on Windows and
  // ~/.local/share on Linux
	fs::path docdir(PlatformUtils::documentsPath());
	add_librarydir(boosty::stringy(docdir / "OpenSCAD" / "libraries"));
#endif

	// This is the built-in read-only library path
	std::string librarydir;
	fs::path libdir(applicationpath);
	fs::path tmpdir;
#ifdef __APPLE__
	libdir /= "../Resources"; // Libraries can be bundled
	if (!is_directory(libdir / "libraries")) libdir /= "../../..";
#elif !defined(WIN32)
	if (is_directory(tmpdir = libdir / "../share/openscad/libraries")) {
		librarydir = boosty::stringy(tmpdir);
	} else if (is_directory(tmpdir = libdir / "../../share/openscad/libraries")) {
		librarydir = boosty::stringy(tmpdir);
	} else if (is_directory(tmpdir = libdir / "../../libraries")) {
		librarydir = boosty::stringy(tmpdir);
	} else
#endif
		if (is_directory(tmpdir = libdir / "libraries")) {
			librarydir = boosty::stringy(tmpdir);
		}
	if (!librarydir.empty()) add_librarydir(librarydir);
}
