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
#include "version.h"
#include "SourceFile.h"
#include "Tree.h"

class SourceFile;

extern bool parse(SourceFile *& file, const std::string& text, const std::string& filename, const std::string& mainFile, int debug);

#include <string>
extern std::string commandline_commands;

// Custom argument parser
std::pair<std::string, std::string> customSyntax(const std::string& s);

int launch_openscad(int argc, char **argv);

SourceFile *parse_scad(std::string text, std::string filename);
// Tree *eval_scad(SourceFile* root_file);
std::shared_ptr<AbstractNode> eval_scad(SourceFile* root_file);
std::shared_ptr<AbstractNode> find_root(std::shared_ptr<AbstractNode> absolute_root_node);
int export_file(std::shared_ptr<AbstractNode> root_node, std::string output_file);

void init_globals();
void set_debug(bool flag);
void set_trace_depth(int value);
void set_quiet(bool flag);
void set_hardwarnings(bool flag);
void set_csglimit(unsigned int limit);
void set_render_color_scheme(const std::string& color_scheme);

bool get_debug();
int get_trace_depth();
bool get_quiet();
bool get_hardwarnings();
unsigned int get_csglimit();
std::string get_render_color_scheme();
