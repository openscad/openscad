#ifndef FILEUTILS_H_
#define FILEUTILS_H_

#include <string>
#include "PlatformUtils.h"

std::string lookup_file(const std::string &filename, const std::string &path, const std::string &fallbackpath);
std::string getfile( PlatformUtils::ifstream &ifs );

#endif
