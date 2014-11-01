#pragma once

#include <string>

namespace PlatformUtils {

	void registerApplicationPath(const std::string &applicationpath);
	std::string applicationPath();

	std::string documentsPath();
	std::string resourcesPath();
	std::string userLibraryPath();
        
        /**
         * Base path where user configuration can be read and written to. On
         * Linux this is the $XDG_CONFIG_HOME.
         * 
         * @return absolute path to the writable configuration folder or
         * an empty string if the config path does not exist.
         */
        std::string userConfigPath();

	bool createUserLibraryPath();
	std::string backupPath();
	bool createBackupPath();

        /**
         * Platform abstraction to set environment variables. Windows/MinGW
         * does not support setenv(), but needs _putenv().
         * 
         * @param name name of the environment variable.
         * @param value the value to set for the environment variable.
         * @return 0 on success.
         */
        int setenv(const char *name, const char *value, int overwrite);
        
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
