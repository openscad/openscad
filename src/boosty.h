/*
#  boosty.h copyright (C) 2011 don bright
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef boosty_h_
#define boosty_h_

/*
 boosty is a wrapper around boost so that OpenSCAD can work with old
 versions of boost found on popular versions of linux, circa early 2012.

 design
  hope that the user is compiling with boost>1.44 + filesystem v3
  if not, fall back to older deprecated functions, and rely on regression
  testing to find bugs. implement the minimum needed by OpenSCAD and no more.
  in a few years, this file should be deleted as unnecessary.

 see also
  http://www.boost.org/doc/libs/1_48_0/libs/filesystem/v3/doc/index.htm
  http://www.boost.org/doc/libs/1_42_0/libs/filesystem/doc/index.htm
  include/boost/wave/util/filesystem_compatability.hpp

*/

#include <string>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace boosty {
#if BOOST_VERSION >= 104600 && BOOST_FILESYSTEM_VERSION >= 3
inline bool is_absolute( fs::path p ) { return p.is_absolute(); }
inline fs::path absolute( fs::path p ) { return fs::absolute( p ); }
inline std::string stringy( fs::path p ) { return p.generic_string(); }
inline std::string extension_str( fs::path p) { return p.extension().generic_string(); }
#else
inline bool is_absolute( fs::path p ) { return p.is_complete(); }
inline fs::path absolute( fs::path p ) { return fs::complete( p ); }
inline std::string stringy( fs::path p ) { return p.string(); }
inline std::string extension_str( fs::path p) { return p.extension(); }
#endif

} // namespace

#endif
