#include "PlatformUtils.h"
#include "boosty.h"
#include <Eigen/Core>

extern std::vector<std::string> librarypath;
extern std::vector<std::string> fontpath;

namespace {
	std::string applicationpath;
}

void PlatformUtils::registerApplicationPath(const std::string &apppath)
{
	applicationpath = apppath;
}

std::string PlatformUtils::applicationPath()
{
	return applicationpath;
}

bool PlatformUtils::createUserLibraryPath()
{
	std::string path = PlatformUtils::userLibraryPath();
	bool OK = false;
	try {
		if (!fs::exists(fs::path(path))) {
			//PRINTB("Creating library folder %s", path );
			OK = fs::create_directories( path );
		}
		if (!OK) {
			PRINTB("ERROR: Cannot create %s", path );
		}
	} catch (const fs::filesystem_error& ex) {
		PRINTB("ERROR: %s",ex.what());
	}
	return OK;
}

std::string PlatformUtils::userLibraryPath()
{
	fs::path path;
	try {
		std::string pathstr = PlatformUtils::documentsPath();
		if (pathstr=="") return "";
		path = boosty::canonical(fs::path( pathstr ));
		//PRINTB("path size %i",boosty::stringy(path).size());
		//PRINTB("lib path found: [%s]", path );
		if (path.empty()) return "";
		path /= "OpenSCAD";
		path /= "libraries";
		//PRINTB("Appended path %s", path );
		//PRINTB("Exists: %i", fs::exists(path) );
	} catch (const fs::filesystem_error& ex) {
		PRINTB("ERROR: %s",ex.what());
	}
	return boosty::stringy( path );
}


std::string PlatformUtils::backupPath()
{
	fs::path path;
	try {
		std::string pathstr = PlatformUtils::documentsPath();
		if (pathstr=="") return "";
		path = boosty::canonical(fs::path( pathstr ));
		if (path.empty()) return "";
		path /= "OpenSCAD";
		path /= "backups";
	} catch (const fs::filesystem_error& ex) {
		PRINTB("ERROR: %s",ex.what());
	}
	return boosty::stringy( path );
}

bool PlatformUtils::createBackupPath()
{
	std::string path = PlatformUtils::backupPath();
	bool OK = false;
	try {
		if (!fs::exists(fs::path(path))) {
			OK = fs::create_directories( path );
		}
		if (!OK) {
			PRINTB("ERROR: Cannot create %s", path );
		}
	} catch (const fs::filesystem_error& ex) {
		PRINTB("ERROR: %s",ex.what());
	}
	return OK;
}

// This is the built-in read-only resources path
std::string PlatformUtils::resourcesPath()
{
	fs::path resourcedir(applicationPath());
	fs::path tmpdir;
#ifdef __APPLE__
	// Resources can be bundled on Mac. If not, fall back to development layout
	bool isbundle = is_directory(resourcedir / ".." / "Resources");
	if (isbundle) {
		resourcedir /= "../Resources";
		// Fall back to dev layout
		if (!is_directory(resourcedir / "libraries")) resourcedir /= "../../..";
	}
#elif !defined(WIN32)
	tmpdir = resourcedir / "../share/openscad";
	if (is_directory(tmpdir / "libraries")) {
		resourcedir = tmpdir;
	}
	else {
		tmpdir = resourcedir / "../../share/openscad";
		if (is_directory(tmpdir / "libraries")) {
			resourcedir = tmpdir;
		} else {
			tmpdir = resourcedir / "../..";
			if (is_directory(tmpdir / "libraries")) {
				resourcedir = tmpdir;
			}
		}
	}
#endif
	// resourcedir defaults to applicationPath
	return boosty::stringy(boosty::canonical(resourcedir));
}
