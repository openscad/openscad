#ifndef PLATFORMUTILS_H_
#define PLATFORMUTILS_H_

#include <string>
#include <fstream>

namespace PlatformUtils {
	std::string documentsPath();
	std::string libraryPath();
	bool createLibraryPath();
	std::string info();
}

#if defined( __MINGW32__ ) || defined( __MINGW64__ )
#define __MINGW_FSTREAM__
namespace PlatformUtils {
	std::string utf16_to_utf8( const std::wstring &w );
	std::wstring utf8_to_utf16( const std::string &s );
}
#include "../patches/mingstream"
namespace PlatformUtils {
	typedef imingstream ifstream;
	typedef omingstream ofstream;
}
#else //mingw
namespace PlatformUtils {
	typedef std::ifstream ifstream;
	typedef std::ofstream ofstream;
}
#endif //mingw

#endif // PLATFORMUTILS_H_
