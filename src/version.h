/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2019 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#pragma once

#include <string_view>

#define QUOTE_(x_) #x_
#define QUOTED_(x__) QUOTE_(x__)

// Version number without any patch level indicator
inline constexpr std::string_view openscad_shortversionnumber = QUOTED_(OPENSCAD_SHORTVERSION);

// The full version number, e.g. 2014.03, 2015.03-1, 2014.12.23
inline constexpr std::string_view openscad_versionnumber = QUOTED_(OPENSCAD_VERSION);

// Version used for display, typically without patchlevel indicator,
// but may include git commit id for snapshot builds
inline constexpr std::string_view openscad_displayversionnumber =
#ifdef OPENSCAD_COMMIT
  QUOTED_(OPENSCAD_VERSION) " (git " QUOTED_(OPENSCAD_COMMIT) ")";
#else
  QUOTED_(OPENSCAD_SHORTVERSION);
#endif

// Version used for detailed display
inline constexpr std::string_view openscad_detailedversionnumber =
#ifdef OPENSCAD_COMMIT
  openscad_displayversionnumber;
#else
  openscad_versionnumber;
#endif

#undef QUOTED_
#undef QUOTE_
