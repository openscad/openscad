// boosty.h by don bright 2012. Copyright assigned to Marius Kintel and
// Clifford Wolf 2012. Released under the GPL 2, or later, as described in
// the file named 'COPYING' in OpenSCAD's project root.

#ifndef boosty_h_
#define boosty_h_

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
namespace fs = boost::filesystem;
#include "printutils.h"

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





#if BOOST_VERSION >= 104800

inline fs::path canonical( fs::path p, fs::path p2 )
{
	return fs::canonical( p, p2 );
}

inline fs::path canonical( fs::path p )
{
	return fs::canonical( p );
}

#else

inline fs::path canonical( fs::path p, fs::path p2 )
{
	// dotpath: win32/mac builds will be using newer versions of boost
	// so we can treat this as though it is unix only
	const fs::path dot_path(".");
	const fs::path dot_dot_path("..");
	fs::path result;
	if (p=="")
	{
		p=p2;
	}
	for (fs::path::iterator itr = p.begin(); itr != p.end(); itr++)
	{
		if (*itr == dot_path) continue;
		if (*itr == dot_dot_path)
		{
			result.remove_filename();
			continue;
		}
		result /= *itr;
		if (fs::is_symlink(result))
		{
			PRINT("WARNING: canonical() wrapper can't do symlinks. upgrade boost to >1.48");
		}
	}
	return result;
}

inline fs::path canonical( fs::path p )
{
	return canonical( p, fs::current_path() );
}

#endif




} // namespace

#endif
