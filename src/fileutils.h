#ifndef FILEUTILS_H_
#define FILEUTILS_H_

#include <string>

std::string lookup_file(const std::string &filename, 
												const std::string &path, const std::string &fallbackpath);

#endif
