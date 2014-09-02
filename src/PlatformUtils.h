#pragma once

#include <string>

namespace PlatformUtils {

	void registerApplicationPath(const std::string &applicationpath);
	std::string applicationPath();

	std::string documentsPath();
	std::string resourcesPath();
	std::string userLibraryPath();
	bool createUserLibraryPath();
	std::string backupPath();
	bool createBackupPath();
  
	/**
	 * Single character separating path specifications in a list
	 * (e.g. OPENSCADPATH). On Windows that's ';' and on most other
	 * systems ':'.
	 * 
	 * @return the path separator
	 */
	std::string pathSeparatorChar();
	
	/* Provide stdout/stderr if not available.
	 * Currently limited to MS Windows GUI application console only.
	 */
	void ensureStdIO(void);
}
