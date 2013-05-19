#ifndef PARSERSETTINGS_H_
#define PARSERSETTINGS_H_

#include <string>
#include "boosty.h"

extern int parser_error_pos;

void parser_init(const std::string &applicationpath);
void add_librarydir(const std::string &libdir);
fs::path search_libs(const std::string &filename);
fs::path find_valid_path( fs::path sourcepath, std::string filename );

#endif
