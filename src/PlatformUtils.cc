#include "PlatformUtils.h"
#include "boosty.h"

bool PlatformUtils::createLibraryPath()
{
	std::string path = PlatformUtils::libraryPath();
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

std::string PlatformUtils::libraryPath()
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

#include "version_check.h"
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
std::string PlatformUtils::info()
{
	std::stringstream s;

#if defined(__GNUG__) && !defined(__clang__)
	std::string compiler_info( "GCC " + std::string(TOSTRING(__VERSION__)) );
#elif defined(_MSC_VER)
	std::string compiler_info( "MSVC " + std::string(TOSTRING(_MSC_FULL_VER)) );
#elif defined(__clang__)
	std::string compiler_info( "Clang " + std::string(TOSTRING(__clang_version__)) );
#else
	std::string compiler_info( "unknown compiler" );
#endif

#if defined( __MINGW64__ )
	std::string mingwstatus("MingW64");
#elif defined( __MINGW32__ )
	std::string mingwstatus("MingW32");
#else
	std::string mingwstatus("No");
#endif

#ifndef OPENCSG_VERSION_STRING
#define OPENCSG_VERSION_STRING "unknown, <1.3.2"
#endif

	s << "OpenSCAD Version: " << TOSTRING(OPENSCAD_VERSION)
          << "\nCompiler: " << compiler_info
	  << "\nCompile date: " << __DATE__
	  << "\nBoost version: " << BOOST_LIB_VERSION
	  << "\nEigen version: " << EIGEN_WORLD_VERSION << "." << EIGEN_MAJOR_VERSION << "." << EIGEN_MINOR_VERSION
	  << "\nCGAL version: " << TOSTRING(CGAL_VERSION)
	  << "\nOpenCSG version: " << OPENCSG_VERSION_STRING
	  << "\nQt version: " << qVersion()
	  << "\nMingW build: " << mingwstatus;

	return s.str();
}

