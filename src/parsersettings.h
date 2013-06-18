#ifndef PARSERSETTINGS_H_
#define PARSERSETTINGS_H_

#include <string>
#include "boosty.h"

extern int parser_error_pos;

void parser_init(const std::string &applicationpath);
void add_librarydir(const std::string &libdir);
fs::path search_libs(const fs::path &localpath);
fs::path find_valid_path(const fs::path &sourcepath, 
												 const fs::path &localpath, 
												 const std::vector<std::string> *openfilenames = NULL);

#endif
