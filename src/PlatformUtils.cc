#include "PlatformUtils.h"
#include "boosty.h"

std::string PlatformUtils::libraryPath()
{
	fs::path path;
	bool OK = true;
	try {
		path = boosty::canonical(fs::path(PlatformUtils::documentsPath()));
		if (path.empty()) return "";
		PRINTB("path size %i",boosty::stringy(path).size());
		path /= "OpenSCAD";
		path /= "libraries";
		PRINTB("Appended path %s", path );
		PRINTB("Exists: %i", fs::exists(path) );
		if (!fs::exists(path)) {
			PRINTB("Creating library folder %s", boosty::stringy(path) );
			OK &= fs::create_directories( path );
		}
		if (!OK) {
			PRINTB("ERROR: Cannot find nor create %s", boosty::stringy(path) );
			path = fs::path("");
		}
	} catch (const fs::filesystem_error& ex) {
		PRINTB("ERROR: %s",ex.what());
	}
	return boosty::stringy( path );
}
