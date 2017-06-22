#pragma once

#include <string>

#include <boost/filesystem.hpp>
namespace fs=boost::filesystem;

#define STACK_BUFFER_SIZE (64 * 1024)
#define STACK_LIMIT_DEFAULT (STACKSIZE - STACK_BUFFER_SIZE)

namespace PlatformUtils {
        extern const char *OPENSCAD_FOLDER_NAME;

	void registerApplicationPath(const std::string &applicationpath);
	std::string applicationPath();

	std::string documentsPath();
        std::string resourceBasePath();
	fs::path resourcePath(const std::string& resource);
	std::string userLibraryPath();
        
        /**
         * Base path where user configuration can be read and written to. On
         * Linux this is the $XDG_CONFIG_HOME, on Windows the local AppData
         * folder CSIDL_LOCAL_APPDATA.
         * On success the returned path can be used directly as base folder
         * as it already includes an OpenSCAD specific part.
         * 
         * @return absolute path to the writable configuration folder or
         * an empty string if the config path does not exist.
         */
        std::string userConfigPath();

	bool createUserLibraryPath();
	std::string backupPath();
	bool createBackupPath();

        /**
         * Return a human readable text describing the operating system
         * the application is currently running on. This is mainly intended
         * to provide information for bug reports (e.g. to be included in
         * the LibraryInfoDialog).
         * 
         * If there is some error to retrieve the details, at least the
         * OS type is reported based on what platform the application was
         * built for.
         * 
         * Extended sysinfo will return more info, like CPUs and RAM
         * @return system information.
         */
        std::string sysinfo(bool extended = true);

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
         * Return system defined stack limit. If the system does not define
         * a specific limit, the platform specific code will select a value.
         * 
         * @return maximum stack size in bytes.
         */
        unsigned long stackLimit();
        
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
        
        /**
         * Convert the number of bytes to a human readable string with
         * a given number of digits.
         */
        std::string toMemorySizeString(uint64_t bytes, int digits);
}
