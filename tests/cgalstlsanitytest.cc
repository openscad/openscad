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
#include "context.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "Tree.h"
#include "CGAL_Nef_polyhedron.h"
#include "CGALEvaluator.h"
#include "PolySetCGALEvaluator.h"

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QSet>
#include <QTextStream>
#ifndef _MSC_VER
#include <getopt.h>
#endif
#include <iostream>
#include <assert.h>
#include <sstream>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "boosty.h"

std::string commandline_commands;
std::string currentdir;
QString examplesdir;

using std::string;

void cgalTree(Tree &tree)
{
	assert(tree.root());

	CGALEvaluator evaluator(tree);
	Traverser evaluate(evaluator, *tree.root(), Traverser::PRE_AND_POSTFIX);
	evaluate.execute();
}

AbstractNode *find_root_tag(AbstractNode *n)
{
	foreach(AbstractNode *v, n->children) {
		if (v->modinst->tag_root) return v;
		if (AbstractNode *vroot = find_root_tag(v)) return vroot;
	}
	return NULL;
}

int main(int argc, char **argv)
{
	int retval = 0;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <file.scad> <output.txt>\n", argv[0]);
		exit(1);
	}

	const char *filename = argv[1];
	const char *outfilename = argv[2];

	Builtins::instance()->initialize();

	QCoreApplication app(argc, argv);
	fs::path original_path = fs::current_path();

	currentdir = boosty::stringy( fs::current_path() );

	parser_init(QCoreApplication::instance()->applicationDirPath().toStdString());
	set_librarydir(boosty::stringy(fs::path(QCoreApplication::instance()->applicationDirPath().toStdString()) / "../libraries"));

	Context root_ctx;
	register_builtin(root_ctx);

	AbstractModule *root_module;
	ModuleInstantiation root_inst;

	root_module = parsefile(filename);
	if (!root_module) {
		exit(1);
	}

	fs::current_path(fs::path(filename).parent_path());

	AbstractNode::resetIndexCounter();
	AbstractNode *absolute_root_node = root_module->evaluate(&root_ctx, &root_inst);
	AbstractNode *root_node;
	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node))) root_node = absolute_root_node;

	Tree tree(root_node);

	CGALEvaluator cgalevaluator(tree);
 	PolySetCGALEvaluator psevaluator(cgalevaluator);

	CGAL_Nef_polyhedron N = cgalevaluator.evaluateCGALMesh(*root_node);

	current_path(original_path);
	if (!N.empty()) {
		std::ofstream outfile;
		outfile.open(outfilename);

		std::stringstream out;
		export_stl(&N, out);
		if (out.str().find("nan") != string::npos) {
			outfile << "Error: nan found\n";
			retval = 2;
		}
		if (out.str().find("inf") != string::npos) {
			outfile << "Error: inf found\n";
			retval = 2;
		}
		outfile.close();
	}

	Builtins::instance(true);

	return retval;
}
