#include "LibraryInfo.h"
#include <glib.h>
#include <vector>
#ifdef USE_SCINTILLA_EDITOR
#include <Qsci/qsciglobal.h>
#include "input/InputDriverManager.h"
#endif

#include "version_check.h"
#include "PlatformUtils.h"
#include "openscad.h"
#include "version.h"
#include "feature.h"

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

#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#ifndef OPENCSG_VERSION_STRING
#define OPENCSG_VERSION_STRING "unknown, < 1.3.2"
#endif
#else
#define OPENCSG_VERSION_STRING "<not enabled>"
#endif

extern std::vector<std::string> librarypath;
extern std::vector<std::string> fontpath;
extern const std::string get_cairo_version();
extern const std::string get_lib3mf_version();
extern const std::string get_fontconfig_version();
extern const std::string get_harfbuzz_version();
extern const std::string get_freetype_version();
extern const char * LODEPNG_VERSION_STRING;

std::string LibraryInfo::info()
{
	std::ostringstream s;

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

#ifdef DEBUG
	std::string debugstatus("Yes");
#else
	std::string debugstatus("No");
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
	cgal_3d_kernel = std::string( abi::__cxa_demangle( cgal_3d_kernel.c_str(), nullptr, nullptr, &status ) );
	cgal_2d_kernel = std::string( abi::__cxa_demangle( cgal_2d_kernel.c_str(), nullptr, nullptr, &status ) );
	cgal_2d_kernelEx = std::string( abi::__cxa_demangle( cgal_2d_kernelEx.c_str(), nullptr, nullptr, &status ) );
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
	  << "\nUser Agent: " << PlatformUtils::user_agent()
	  << "\nCompiler: " << compiler_info
	  << "\nMinGW build: " << mingwstatus
	  << "\nDebug build: " << debugstatus
	  << "\nBoost version: " << BOOST_LIB_VERSION
	  << "\nEigen version: " << EIGEN_WORLD_VERSION << "." << EIGEN_MAJOR_VERSION << "." << EIGEN_MINOR_VERSION
	  << "\nCGAL version, kernels: " << TOSTRING(CGAL_VERSION) << ", " << cgal_3d_kernel << ", " << cgal_2d_kernel << ", " << cgal_2d_kernelEx
	  << "\nOpenCSG version: " << OPENCSG_VERSION_STRING
	  << "\nQt version: " << qtVersion
#ifdef USE_SCINTILLA_EDITOR
	  << "\nQScintilla version: " << QSCINTILLA_VERSION_STR
          << "\nInputDrivers: " << InputDriverManager::instance()->listDrivers()
#endif
	  << "\nGLib version: "       << GLIB_MAJOR_VERSION << "." << GLIB_MINOR_VERSION << "." << GLIB_MICRO_VERSION
	  << "\nlodepng version: " << LODEPNG_VERSION_STRING
	  << "\nlibzip version: " << LIBZIP_VERSION
	  << "\nfontconfig version: " << get_fontconfig_version()
	  << "\nfreetype version: " << get_freetype_version()
	  << "\nharfbuzz version: " << get_harfbuzz_version()
	  << "\ncairo version: " << get_cairo_version()
	  << "\nlib3mf version: " << get_lib3mf_version()
#ifdef ENABLE_EXPERIMENTAL
	  << "\nFeatures: " << Feature::features()
#endif	  
		<< "\nApplication Path: " << PlatformUtils::applicationPath()
	  << "\nDocuments Path: " << PlatformUtils::documentsPath()
	  << "\nUser Documents Path: " << PlatformUtils::userDocumentsPath()
	  << "\nResource Path: " << PlatformUtils::resourceBasePath()
	  << "\nUser Library Path: " << PlatformUtils::userLibraryPath()
	  << "\nUser Config Path: " << PlatformUtils::userConfigPath()
	  << "\nBackup Path: " << PlatformUtils::backupPath()
	  << "\nOPENSCADPATH: " << (env_path == nullptr ? "<not set>" : env_path)
	  << "\nOpenSCAD library path:\n";

	for (const auto &path : librarypath) {
		s << "  " << path << "\n";
	}

	s << "\nOPENSCAD_FONT_PATH: " << (env_font_path == nullptr ? "<not set>" : env_font_path)
	  << "\nOpenSCAD font path:\n";
	
	for (const auto &path : fontpath) {
		s << "  " << path << "\n";
	}

	return s.str();
}
