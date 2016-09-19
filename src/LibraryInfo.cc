#include "LibraryInfo.h"
#include <glib.h>
#include <vector>
#ifdef USE_SCINTILLA_EDITOR
#include <Qsci/qsciglobal.h>
#endif

#include "version_check.h"
#include "PlatformUtils.h"
#include "openscad.h"
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef ENABLE_CGAL
#include "cgal.h"
#include <boost/algorithm/string.hpp>
#if defined(__GNUG__)
#define GCC_INT_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 )
#if GCC_INT_VERSION > 40600 || defined(__clang__)
#include <cxxabi.h>
#define __openscad_info_demangle__ 1
#endif // GCC_INT_VERSION
#endif // GNUG
#endif // ENABLE_CGAL

#ifdef ENABLE_LIBZIP
#include <zip.h>
#else
#define LIBZIP_VERSION "<not enabled>"
#endif

extern std::vector<std::string> librarypath;
extern std::vector<std::string> fontpath;

std::string LibraryInfo::info()
{
	std::stringstream s;

#if defined(__x86_64__) || defined(_M_X64)
	std::string bits(" 64bit");
#elif defined(__i386) || defined(_M_IX86)
	std::string bits(" 32bit");
#else
	std::string bits("");
#endif
	
#if defined(__GNUG__) && !defined(__clang__)
	std::string compiler_info( "GCC " + std::string(TOSTRING(__VERSION__)) + bits);
#elif defined(_MSC_VER)
	std::string compiler_info( "MSVC " + std::string(TOSTRING(_MSC_FULL_VER)) + bits);
#elif defined(__clang__)
	std::string compiler_info( "Clang " + std::string(TOSTRING(__clang_version__)) + bits);
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

#ifdef QT_VERSION
	std::string qtVersion = qVersion();
#else
	std::string qtVersion = "Qt disabled - Commandline Test Version";
#endif

#ifdef ENABLE_CGAL
	std::string cgal_3d_kernel = typeid(CGAL_Kernel3).name();
	std::string cgal_2d_kernel = typeid(CGAL_Kernel2).name();
	std::string cgal_2d_kernelEx = typeid(CGAL_ExactKernel2).name();
#if defined(__openscad_info_demangle__)
	int status;
	cgal_3d_kernel = std::string( abi::__cxa_demangle( cgal_3d_kernel.c_str(), 0, 0, &status ) );
	cgal_2d_kernel = std::string( abi::__cxa_demangle( cgal_2d_kernel.c_str(), 0, 0, &status ) );
	cgal_2d_kernelEx = std::string( abi::__cxa_demangle( cgal_2d_kernelEx.c_str(), 0, 0, &status ) );
#endif // demangle
	boost::replace_all( cgal_3d_kernel, "CGAL::", "" );
	boost::replace_all( cgal_2d_kernel, "CGAL::", "" );
	boost::replace_all( cgal_2d_kernelEx, "CGAL::", "" );
#else // ENABLE_CGAL
	std::string cgal_3d_kernel = "";
	std::string cgal_2d_kernel = "";
	std::string cgal_2d_kernelEx = "";
#endif // ENABLE_CGAL

	const char *env_path = getenv("OPENSCADPATH");
	const char *env_font_path = getenv("OPENSCAD_FONT_PATH");
	
	s << "OpenSCAD Version: " << openscad_detailedversionnumber
	  << "\nSystem information: " << PlatformUtils::sysinfo()
		<< "\nCompiler: " << compiler_info
	  << "\nBoost version: " << BOOST_LIB_VERSION
	  << "\nEigen version: " << EIGEN_WORLD_VERSION << "." << EIGEN_MAJOR_VERSION << "." << EIGEN_MINOR_VERSION
	  << "\nCGAL version, kernels: " << TOSTRING(CGAL_VERSION) << ", " << cgal_3d_kernel << ", " << cgal_2d_kernel << ", " << cgal_2d_kernelEx
	  << "\nOpenCSG version: " << OPENCSG_VERSION_STRING
	  << "\nQt version: " << qtVersion
#ifdef USE_SCINTILLA_EDITOR
	  << "\nQScintilla version: " << QSCINTILLA_VERSION_STR
#endif
	  << "\nMingW build: " << mingwstatus
	  << "\nGLib version: "       << GLIB_MAJOR_VERSION << "." << GLIB_MINOR_VERSION << "." << GLIB_MICRO_VERSION
	  << "\nlibzip version: " << LIBZIP_VERSION
	  << "\nApplication Path: " << PlatformUtils::applicationPath()
	  << "\nDocuments Path: " << PlatformUtils::documentsPath()
	  << "\nResource Path: " << PlatformUtils::resourceBasePath()
	  << "\nUser Library Path: " << PlatformUtils::userLibraryPath()
	  << "\nUser Config Path: " << PlatformUtils::userConfigPath()
	  << "\nBackup Path: " << PlatformUtils::backupPath()
	  << "\nOPENSCADPATH: " << (env_path == NULL ? "<not set>" : env_path)
	  << "\nOpenSCAD library path:\n";

	for (std::vector<std::string>::iterator it = librarypath.begin();it != librarypath.end();it++) {
		s << "  " << *it << "\n";
	}

	s << "\nOPENSCAD_FONT_PATH: " << (env_font_path == NULL ? "<not set>" : env_font_path)
	  << "\nOpenSCAD font path:\n";
	
	for (std::vector<std::string>::iterator it = fontpath.begin();it != fontpath.end();it++) {
		s << "  " << *it << "\n";
	}

	return s.str();
}
