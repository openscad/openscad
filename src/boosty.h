// boosty.h copyright 2012 don bright. released under the GPL 2, or later,
// as described in the file named 'COPYING' in OpenSCAD's project root.
// permission is given to Marius Kintel & Clifford Wolf to change this license.

#ifndef boosty_h_
#define boosty_h_

/*
 boosty is a wrapper around boost so that OpenSCAD can work with old
 versions of boost found on popular versions of linux, circa early 2012.

 design
  hope that the user is compiling with boost>1.46 + filesystem v3
  if not, fall back to older deprecated functions, and rely on
  testing to find bugs. implement the minimum needed by OpenSCAD and no more.
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
namespace fs = boost::filesystem;

namespace boosty {

#if BOOST_VERSION >= 104400 && BOOST_FILESYSTEM_VERSION >= 3

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

#else

inline bool is_absolute( fs::path p )
{
	return p.is_complete();
}

inline fs::path absolute( fs::path p )
{
	return fs::complete(p, fs::current_path());
}

inline std::string stringy( fs::path p )
{
	return p.string();
}

inline std::string extension_str( fs::path p)
{
	return p.extension();
}

#endif

} // namespace

#endif
