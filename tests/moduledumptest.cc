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

#include "tests-common.h"
#include "openscad.h"
#include "parsersettings.h"
#include "node.h"
#include "module.h"
#include "modcontext.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "Tree.h"

#ifndef _MSC_VER
#include <getopt.h>
#endif
#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "boosty.h"

std::string commandline_commands;
std::string currentdir;

using std::string;

string dumptree(const Tree &tree, const AbstractNode &node)
{
	std::stringstream str;
	const std::vector<AbstractNode*> &children = node.getChildren();
	for (std::vector<AbstractNode*>::const_iterator iter = children.begin(); iter != children.end(); iter++) {
		str << tree.getString(**iter) << "\n";
	}
	return str.str();
}

int main(int argc, char **argv)
{
#ifdef _MSC_VER
  _set_output_format(_TWO_DIGIT_EXPONENT);
#endif
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <file.scad> <output.txt>\n", argv[0]);
		exit(1);
	}

	const char *filename = argv[1];
	const char *outfilename = argv[2];
	int rc = 0;

	Builtins::instance()->initialize();

	fs::path original_path = fs::current_path();

	currentdir = boosty::stringy(fs::current_path());

	parser_init(boosty::stringy(fs::path(argv[0]).branch_path()));
	add_librarydir(boosty::stringy(fs::path(argv[0]).branch_path() / "../libraries"));

	ModuleContext top_ctx;
	top_ctx.registerBuiltin();

	FileModule *root_module;
	ModuleInstantiation root_inst("group");

	root_module = parsefile(filename);
	if (!root_module) {
		exit(1);
	}

	string dumpstdstr = root_module->dump("", "");

	fs::current_path(original_path);
	std::ofstream outfile;
	outfile.open(outfilename);
	if (!outfile.is_open()) {
		fprintf(stderr, "Error: Unable to open output file %s\n", outfilename);
		exit(1);
	}
	std::cout << "Opened " << outfilename << "\n";
	outfile << dumpstdstr << "\n";
	outfile.close();
	if (outfile.fail()) fprintf(stderr, "Failed to close file\n");

	delete root_module;

	Builtins::instance(true);

	return rc;
}
