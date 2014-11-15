#include <stdlib.h>

#include "PlatformUtils.h"
#include "boosty.h"
#include <Eigen/Core>
#ifdef USE_SCINTILLA_EDITOR
#include <Qsci/qsciglobal.h>
#endif

extern std::vector<std::string> librarypath;
extern std::vector<std::string> fontpath;

namespace {
	std::string applicationpath;
	std::string resourcespath;
}

const char *PlatformUtils::OPENSCAD_FOLDER_NAME = "OpenSCAD";

static std::string lookupResourcesPath()
{
	fs::path resourcedir(applicationpath);
	PRINTDB("Looking up resource folder with application path '%s'", resourcedir.c_str());
	
#ifndef WIN32
#ifdef __APPLE__
	const char *searchpath[] = {
	    "../Resources", 	// Resources can be bundled on Mac.
	    "../../..",       // Dev location
	    "..",          // Test location
	    NULL
	};
#else
	const char *searchpath[] = {
	    "../share/openscad",
	    "../../share/openscad",
	    ".",
	    "..",
	    "../..",
	    NULL
	};
#endif	

	fs::path tmpdir;
	for (int a = 0;searchpath[a] != NULL;a++) {
	    tmpdir = resourcedir / searchpath[a];
	    
	    const fs::path checkdir = tmpdir / "libraries";
	    PRINTDB("Checking '%s'", checkdir.c_str());

	    if (is_directory(checkdir)) {
		resourcedir = tmpdir;
		PRINTDB("Found resource folder '%s'", tmpdir.c_str());
		break;
	    }
	}
#endif // !WIN32

	// resourcedir defaults to applicationPath
	std::string result = boosty::stringy(boosty::canonical(resourcedir));
	PRINTDB("Using resource folder '%s'", result);
	return result;
}

void PlatformUtils::registerApplicationPath(const std::string &apppath)
{
	applicationpath = apppath;
	resourcespath = lookupResourcesPath();
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
		path /= OPENSCAD_FOLDER_NAME;
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
		path /= OPENSCAD_FOLDER_NAME;
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
    return resourcespath;
}

int PlatformUtils::setenv(const char *name, const char *value, int overwrite)
{
#if defined(WIN32)
    const char *ptr = getenv(name);
    if ((overwrite == 0) && (ptr != NULL)) {
	return 0;
    }

    char buf[4096];
    snprintf(buf, sizeof(buf), "%s=%s", name, value);
    return _putenv(buf);
#else
    return ::setenv(name, value, overwrite);
#endif
}
