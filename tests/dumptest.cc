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
#include "node.h"
#include "module.h"
#include "context.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "Tree.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSet>
#ifndef _MSC_VER
#include <getopt.h>
#endif
#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>

using std::string;

std::string commandline_commands;
QString currentdir;
QString examplesdir;
QString librarydir;

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
#ifdef WIN32
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

	QApplication app(argc, argv, false);
	QDir original_path = QDir::current();

	currentdir = QDir::currentPath();

	QDir libdir(QApplication::instance()->applicationDirPath());
#ifdef Q_WS_MAC
	libdir.cd("../Resources"); // Libraries can be bundled
	if (!libdir.exists("libraries")) libdir.cd("../../..");
#elif defined(Q_OS_UNIX)
	if (libdir.cd("../share/openscad/libraries")) {
		librarydir = libdir.path();
	} else
	if (libdir.cd("../../share/openscad/libraries")) {
		librarydir = libdir.path();
	} else
	if (libdir.cd("../../libraries")) {
		librarydir = libdir.path();
	} else
#endif
	if (libdir.cd("libraries")) {
		librarydir = libdir.path();
	}

	Context root_ctx;
	register_builtin(root_ctx);

	AbstractModule *root_module;
	ModuleInstantiation root_inst;
	AbstractNode *root_node;

	root_module = parsefile(filename);
	if (!root_module) {
		exit(1);
	}

	QFileInfo fileInfo(filename);
	QDir::setCurrent(fileInfo.absolutePath());

	AbstractNode::resetIndexCounter();
	root_node = root_module->evaluate(&root_ctx, &root_inst);

	Tree tree;
	tree.setRoot(root_node);

	string dumpstdstr = dumptree(tree, *root_node);
	string dumpstdstr_cached = dumptree(tree, *root_node);
	if (dumpstdstr != dumpstdstr_cached) {
		fprintf(stderr, "Error: Dump cached failed\n");
		exit(1);
	}

	QDir::setCurrent(original_path.absolutePath());
	std::ofstream outfile;
	outfile.open(outfilename);
	outfile << dumpstdstr << "\n";
	outfile.close();

	root_module = parsefile(outfilename);
	if (!root_module) {
		fprintf(stderr, "Error: Unable to read back dumped file\n");
		exit(1);
	}
	fileInfo = QFileInfo(outfilename);
	QDir::setCurrent(fileInfo.absolutePath());

	AbstractNode::resetIndexCounter();
	root_node = root_module->evaluate(&root_ctx, &root_inst);

	tree.setRoot(root_node);

	string readbackstr = dumptree(tree, *root_node);
	if (dumpstdstr != readbackstr) {
		fprintf(stderr, "Error: Readback is different from original dump:\n");
		fprintf(stderr, "Original:\n");
		fprintf(stderr, dumpstdstr.c_str());
		fprintf(stderr, "Readback:\n");
		fprintf(stderr, readbackstr.c_str());
		exit(1);
	}

	Builtins::instance(true);

	return rc;
}
