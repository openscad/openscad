#pragma once

#include <string>

namespace PlatformUtils {

	std::string documentsPath();
	std::string libraryPath();
	bool createLibraryPath();
	std::string backupPath();
	bool createBackupPath();
	std::string info();
        
        /**
         * Single character separating path specifications in a list
         * (e.g. OPENSCADPATH). On Windows that's ';' and on most other
         * systems ':'.
         * 
         * @return the path separator
         */
        std::string pathSeparatorChar();
}
