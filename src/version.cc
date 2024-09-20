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

#include "version.h"

#include <string>

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

std::string openscad_shortversionnumber = QUOTED(OPENSCAD_SHORTVERSION);
std::string openscad_versionnumber = QUOTED(OPENSCAD_VERSION);

std::string openscad_displayversionnumber =
#ifdef OPENSCAD_COMMIT
  QUOTED(OPENSCAD_VERSION)
  " (git " QUOTED(OPENSCAD_COMMIT) ")";
#else
  QUOTED(OPENSCAD_SHORTVERSION);
#endif

std::string openscad_detailedversionnumber =
#ifdef OPENSCAD_COMMIT
  openscad_displayversionnumber;
#else
  openscad_versionnumber;
#endif
