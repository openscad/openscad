/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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

#ifndef OPENSCAD_H
#define OPENSCAD_H

#ifdef ENABLE_OPENCSG
// this must be included before the GL headers
#  include <GL/glew.h>
#endif

// for win32 and maybe others..
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif


extern class AbstractModule *parse(const char *text, const char *path, int debug);
extern int get_fragments_from_r(double r, double fn, double fs, double fa);

#include <QString>
extern QString commandline_commands;
extern int parser_error_pos;

extern void handle_dep(QString filename);

extern QString examplesdir;
extern QString librarydir;

#endif

