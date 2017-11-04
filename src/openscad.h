/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
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

#include <boost/filesystem.hpp>

extern bool parse(class FileModule * &module, const char *text, const boost::filesystem::path &filename, int debug);

#include <string>
extern std::string commandline_commands;

// The CWD when application started. We shouldn't change CWD, but until we stop
// doing this, use currentdir to get the original CWD.
extern std::string currentdir;

// Version number without any patch level indicator
extern std::string openscad_shortversionnumber;
// The full version number, e.g. 2014.03, 2015.03-1, 2014.12.23
extern std::string openscad_versionnumber;
// Version used for display, typically without patchlevel indicator,
// but may include git commit id for snapshot builds
extern std::string openscad_displayversionnumber;
// Version used for detailed display
extern std::string openscad_detailedversionnumber;
// Custom argument parser
std::pair<std::string, std::string> customSyntax(const std::string &s);
