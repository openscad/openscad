#ifndef PLATFORMUTILS_H_
#define PLATFORMUTILS_H_

#include <string>
#include <fstream>
#include "stl-utils.h"

namespace PlatformUtils {

	std::string documentsPath();
	std::string libraryPath();
	bool createLibraryPath();
	std::string info();
	std::string utf16_to_utf8( const std::wstring &w );
	std::wstring utf8_to_utf16( const std::string &s );

#if !defined( __MINGW32__ ) && !defined ( __MINGW64__ )
	typedef std::ifstream ifstream;
	typedef std::ofstream ofstream;
#endif

}

#if defined( __MINGW32__ ) || defined ( __MINGW64__ )
#include "../patches/mingstream"
#endif

#endif
