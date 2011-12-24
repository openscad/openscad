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
#include "CSGTextRenderer.h"
#include "CSGTextCache.h"
#include "openscad.h"
#include "parsersettings.h"
#include "node.h"
#include "module.h"
#include "context.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "Tree.h"

#include <QApplication>
#ifndef _MSC_VER
#include <getopt.h>
#endif
#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

std::string commandline_commands;
std::string currentdir;
QString examplesdir;

void csgTree(CSGTextCache &cache, const AbstractNode &root)
{
	CSGTextRenderer renderer(cache);
	Traverser render(renderer, root, Traverser::PRE_AND_POSTFIX);
	render.execute();
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <file.scad> <output.txt>\n", argv[0]);
		exit(1);
	}

	const char *filename = argv[1];
	const char *outfilename = argv[2];

	int rc = 0;

	Builtins::instance()->initialize();

	QApplication app(argc, argv, false);
	fs::path original_path = fs::current_path();

	currentdir = fs::current_path().generic_string();

	parser_init();

	Context root_ctx;
	register_builtin(root_ctx);

	AbstractModule *root_module;
	ModuleInstantiation root_inst;
	AbstractNode *root_node;

	root_module = parsefile(filename);
	if (!root_module) {
		exit(1);
	}

	fs::current_path(fs::path(filename).parent_path());

	AbstractNode::resetIndexCounter();
	root_node = root_module->evaluate(&root_ctx, &root_inst);

	Tree tree;
	tree.setRoot(root_node);
	CSGTextCache csgcache(tree);

	csgTree(csgcache, *root_node);
// 	std::cout << tree.getString(*root_node) << "\n";

	current_path(original_path);
	std::ofstream outfile;
	outfile.open(outfilename);
	outfile << csgcache[*root_node] << "\n";
	outfile.close();

	delete root_node;
	delete root_module;

	Builtins::instance(true);

	return rc;
}
