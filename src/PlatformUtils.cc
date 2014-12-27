#include <stdlib.h>
#include <iomanip>

#include "PlatformUtils.h"

#ifdef INSTALL_SUFFIX
#define RESOURCE_FOLDER(path) path INSTALL_SUFFIX
#else
#define RESOURCE_FOLDER(path) path
#endif

extern std::vector<std::string> librarypath;
extern std::vector<std::string> fontpath;

namespace {
	bool path_initialized = false;
	std::string applicationpath;
	std::string resourcespath;
}

const char *PlatformUtils::OPENSCAD_FOLDER_NAME = "OpenSCAD";

static std::string lookupResourcesPath()
{
	fs::path resourcedir(applicationpath);
	PRINTDB("Looking up resource folder with application path '%s'", resourcedir.c_str());
	
#ifdef __APPLE__
	const char *searchpath[] = {
	    "../Resources", 	// Resources can be bundled on Mac.
	    "../../..",       // Dev location
	    "..",          // Test location
	    NULL
	};
#else
#ifdef WIN32
    const char *searchpath[] = {
        ".", // Release location
        RESOURCE_FOLDER("../share/openscad"), // MSYS2 location
        "..", // Dev location
        NULL
    };
#else
    const char *searchpath[] = {
	    RESOURCE_FOLDER("../share/openscad"),
	    RESOURCE_FOLDER("../../share/openscad"),
	    ".",
	    "..",
	    "../..",
	    NULL
	};
#endif	
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

	// resourcedir defaults to applicationPath
	std::string result = boosty::stringy(boosty::canonical(resourcedir));
	PRINTDB("Using resource folder '%s'", result);
	return result;
}

void PlatformUtils::registerApplicationPath(const std::string &apppath)
{
	applicationpath = apppath;
	resourcespath = lookupResourcesPath();
	path_initialized = true;
}

std::string PlatformUtils::applicationPath()
{
	if (!path_initialized) {
	    throw std::runtime_error("PlatformUtils::applicationPath(): application path not initialized!");
	}
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
std::string PlatformUtils::resourceBasePath()
{
	if (!path_initialized) {
	    throw std::runtime_error("PlatformUtils::resourcesPath(): application path not initialized!");
	}
	return resourcespath;
}

fs::path PlatformUtils::resourcePath(const std::string &resource)
{
	fs::path base(resourceBasePath());
	if (!fs::is_directory(base)) {
		return fs::path();
	}
	
	fs::path resource_dir = base / resource;
	if (!fs::is_directory(resource_dir)) {
		return fs::path();
	}
	
	return resource_dir;
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

std::string PlatformUtils::toMemorySizeString(uint64_t bytes, int digits)
{
	static const char *units[] = { "B", "kB", "MB", "GB", "TB", NULL };
	
	int idx = 0;
	double val = bytes;
	while (true) {
		if (val < 1024.0) {
			break;
		}
		if (units[idx + 1] == NULL) {
			break;
		}
		idx++;
		val /= 1024.0;
	}
	
	boost::format fmt("%f %s");
	fmt % boost::io::group(std::setprecision(digits), val) % units[idx];
	return fmt.str();
}
