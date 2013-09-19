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
#include "cgal.h"
#include <boost/algorithm/string.hpp>
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#if defined(__GNUG__)
#define GCC_INT_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 )
#if GCC_INT_VERSION > 40700 || defined(__clang__)
#include <cxxabi.h>
#define __openscad_info_demangle__ 1
#endif // GCC_INT_VERSION
#endif // GNUG

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

	std::string cgal_3d_kernel = typeid(CGAL_Kernel3).name();
	std::string cgal_2d_kernel = typeid(CGAL_Kernel2).name();
	std::string cgal_2d_kernelEx = typeid(CGAL_ExactKernel2).name();
#if defined(__openscad_info_demangle__)
	int status;
	cgal_3d_kernel = std::string( abi::__cxa_demangle( cgal_3d_kernel.c_str(), 0, 0, &status ) );
	cgal_2d_kernel = std::string( abi::__cxa_demangle( cgal_2d_kernel.c_str(), 0, 0, &status ) );
	cgal_2d_kernelEx = std::string( abi::__cxa_demangle( cgal_2d_kernelEx.c_str(), 0, 0, &status ) );
#endif
	boost::replace_all( cgal_3d_kernel, "CGAL::", "" );
	boost::replace_all( cgal_2d_kernel, "CGAL::", "" );
	boost::replace_all( cgal_2d_kernelEx, "CGAL::", "" );

	s << "OpenSCAD Version: " << TOSTRING(OPENSCAD_VERSION)
          << "\nCompiler: " << compiler_info
	  << "\nCompile date: " << __DATE__
	  << "\nBoost version: " << BOOST_LIB_VERSION
	  << "\nEigen version: " << EIGEN_WORLD_VERSION << "." << EIGEN_MAJOR_VERSION << "." << EIGEN_MINOR_VERSION
	  << "\nCGAL version, kernels: " << TOSTRING(CGAL_VERSION) << ", " << cgal_3d_kernel << ", " << cgal_2d_kernel << ", " << cgal_2d_kernelEx
	  << "\nOpenCSG version: " << OPENCSG_VERSION_STRING
	  << "\nQt version: " << qVersion()
	  << "\nMingW build: " << mingwstatus
	;
	return s.str();
}

