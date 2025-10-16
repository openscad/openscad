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

#include <string>
#include <utility>

#ifdef _WIN32
    #define OPENSCAD_API __declspec(dllexport)
#else
    #define OPENSCAD_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

  OPENSCAD_API int openscad_cmdline(int argc, const char* argv[]);

  OPENSCAD_API int openscad_init();


#ifdef __cplusplus
}
#endif

extern bool parse(class SourceFile *& file, const std::string& text, const std::string& filename,
                  const std::string& mainFile, int debug);

extern std::string commandline_commands;

// Custom argument parser
std::pair<std::string, std::string> customSyntax(const std::string& s);

void localization_init();
void set_render_color_scheme(const std::string& color_scheme, const bool exit_if_not_found);
