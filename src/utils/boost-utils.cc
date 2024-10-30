#include "utils/boost-utils.h"
#include <stdexcept>
#include <cstdio>
#include <string>

namespace fs = boost::filesystem;

fs::path
boostfs_uncomplete(fs::path const& p, fs::path const& base)
{
  return fs::relative(p, base == fs::path{} ? fs::path{"."} : base);
}
