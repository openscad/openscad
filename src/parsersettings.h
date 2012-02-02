#ifndef PARSERSETTINGS_H_
#define PARSERSETTINGS_H_

#include <string>

extern int parser_error_pos;

void parser_init(const std::string &applicationpath);
void set_librarydir(const std::string &libdir);
const std::string &get_librarydir();

#endif
