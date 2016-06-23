// boosty.h by don bright 2012. Copyright assigned to Marius Kintel and
// Clifford Wolf 2012. Released under the GPL 2, or later, as described in
// the file named 'COPYING' in OpenSCAD's project root.

#pragma once

/*
 boosty is a wrapper around boost so that OpenSCAD can work with old
 versions of boost found on popular versions of linux, circa early 2012.

 design
  the boost filsystem changed around 1.46-1.48. we do a large #ifdef
  based on boost version that wraps various functions appropriately.
  in a few years, this file should be deleted as unnecessary.

 see also
  http://www.boost.org/doc/libs/1_48_0/libs/filesystem/v3/doc/index.htm
  http://www.boost.org/doc/libs/1_45_0/libs/filesystem/v2/doc/index.htm
  http://www.boost.org/doc/libs/1_42_0/libs/filesystem/doc/index.htm
  http://www.boost.org/doc/libs/1_35_0/libs/filesystem/doc/index.htm
  include/boost/wave/util/filesystem_compatability.hpp

*/

#include <string>
#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
namespace fs = boost::filesystem;
#include "printutils.h"

namespace boosty {

#if BOOST_VERSION >= 104600 && BOOST_FILESYSTEM_VERSION >= 3

inline bool is_absolute( fs::path p )
{
	return p.is_absolute();
}

inline fs::path absolute( fs::path p )
{
	return fs::absolute( p );
}

inline std::string stringy( fs::path p )
{
	return p.generic_string();
}

inline std::string extension_str( fs::path p)
{
	return p.extension().generic_string();
}

inline fs::path canonical( fs::path p, fs::path p2 )
{
	return fs::canonical( p, p2 );
}

inline fs::path canonical( fs::path p )
{
	return fs::canonical( p );
}

#else
#error you should be using a newer version of boost on win/mac/linux
#endif

} // namespace
