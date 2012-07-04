#ifndef PARSERSETTINGS_H_
#define PARSERSETTINGS_H_

#include <string>

extern int parser_error_pos;

void parser_init(const std::string &applicationpath);
void add_librarydir(const std::string &libdir);
std::string locate_file(const std::string &filename);

#endif
