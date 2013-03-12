#ifndef BOOST_UTILS_H_
#define BOOST_UTILS_H_

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
fs::path relativePath(const fs::path &path, const fs::path &relative_to);

#endif
