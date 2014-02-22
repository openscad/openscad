#ifndef HANDLE_DEP_H_
#define HANDLE_DEP_H_

#include <string>
#include <boost/optional.hpp>

extern boost::optional<std::string> make_command;
void handle_dep(const std::string &filename);
bool write_deps(const std::string &filename, const std::string &output_file);

#endif
