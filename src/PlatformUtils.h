#ifndef PLATFORMUTILS_H_
#define PLATFORMUTILS_H_

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

        /**
         * quadrant-conscious arc-tangent. the platform differences come into
         * play when dealing with Not-a-Number (NaN) and Infinity (inf) inputs.
         * See Wikipedia, and compare Linux/Open Group atan2 vs MSVC atan2.
         * We use Linux/Open Group version & test inf input in Regression Tests
         * Note: Untested under MSVC! (as of 2014 Apr)
         *
         * @return the arctangent, in binary floating-point Radians
         */
         double atan2(double y, double x);
}

#endif
