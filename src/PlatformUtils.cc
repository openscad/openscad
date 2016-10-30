#include <stdlib.h>
#include <iomanip>

#include "PlatformUtils.h"
#include "boosty.h"

extern std::vector<std::string> librarypath;
extern std::vector<std::string> fontpath;

namespace {
	bool path_initialized = false;
	std::string applicationpath;
	std::string resourcespath;
}

const char *PlatformUtils::OPENSCAD_FOLDER_NAME = "OpenSCAD";

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)
std::string PlatformUtils::shortversion()
{
	std::vector<std::string> ymd = PlatformUtils::version_ymd();
	if (ymd.size()>2)
		return( boost::format("%s.%s.%s") % ymd[0] % ymd[1] % ymd[2] ).str();
	return( boost::format("%s.%s") % ymd[0] % ymd[1] ).str();
}

std::vector<std::string> PlatformUtils::version_ymd()
{
	std::string ver = PlatformUtils::fullversion();
	std::vector<std::string> ymd,tmp;
	boost::split(tmp, ver, boost::is_any_of("-"));
	boost::split(ymd, tmp[0], boost::is_any_of("."));
	return ymd;
}

std::string PlatformUtils::fullversion()
{
	return std::string(QUOTED(OPENSCAD_VERSION));
}

std::string PlatformUtils::detailedversion()
{
	std::string commit = QUOTED(OPENSCAD_COMMIT);
	return( boost::format("%s %s") % PlatformUtils::fullversion() % commit).str();
}

static std::string lookupResourcesPath()
{
	fs::path resourcedir(applicationpath);
	PRINTDB("Looking up resource folder with application path '%s'", resourcedir.generic_string().c_str());
	
#ifdef __APPLE__
	const char *searchpath[] = {
	    "../Resources", 	// Resources can be bundled on Mac.
	    "../../..",       // Dev location
	    "..",          // Test location
	    NULL
	};
#else
#ifdef _WIN32
    const char *searchpath[] = {
        ".", // Release location
        "../share/openscad", // MSYS2 location
        "..", // Dev location
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
#endif

	fs::path tmpdir;
	for (int a = 0;searchpath[a] != NULL;a++) {
	    tmpdir = resourcedir / searchpath[a];
	    
			// The resource folder is the folder which contains "color-schemes" (as well as 
			// "examples" and "locale", and optionally "libraries" and "fonts")
	    const fs::path checkdir = tmpdir / "color-schemes";
	    PRINTDB("Checking '%s'", checkdir.generic_string().c_str());

	    if (is_directory(checkdir)) {
		resourcedir = tmpdir;
		PRINTDB("Found resource folder '%s'", tmpdir.generic_string().c_str());
		break;
	    }
	}

	// resourcedir defaults to applicationPath
	std::string result = boosty::canonical(resourcedir).generic_string();
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
		path = fs::path( pathstr );
		if (!fs::exists(path)) return "";
		path = boosty::canonical( path );
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
	return path.generic_string();
}


std::string PlatformUtils::backupPath()
{
	fs::path path;
	try {
		std::string pathstr = PlatformUtils::documentsPath();
		if (pathstr=="") return "";
		path = fs::path( pathstr );
		if (!fs::exists(path)) return "";
		path = boosty::canonical( path );
		if (path.empty()) return "";
		path /= OPENSCAD_FOLDER_NAME;
		path /= "backups";
	} catch (const fs::filesystem_error& ex) {
		PRINTB("ERROR: %s",ex.what());
	}
	return path.generic_string();
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
#if defined(_WIN32)
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
