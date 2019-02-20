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

#include "myqhash.h"
#include "openscad.h"
#include "node.h"
#include "module.h"
#include "context.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "Tree.h"
#include "CGALRenderer.h"
#include "PolySetCGALRenderer.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSet>
#include <QTextStream>
#include <getopt.h>
#include <iostream>

QString commandline_commands;
const char *make_command = NULL;
QSet<QString> dependencies;
QString currentdir;
QString examplesdir;
QString librarydir;

using std::string;

void handle_dep(QString filename)
{
	if (filename.startsWith("/"))
		dependencies.insert(filename);
	else
		dependencies.insert(QDir::currentPath() + QString("/") + filename);
	if (!QFile(filename).exists() && make_command) {
		char buffer[4096];
		snprintf(buffer, 4096, "%s '%s'", make_command, filename.replace("'", "'\\''").toUtf8().data());
		system(buffer); // FIXME: Handle error
	}
}

// FIXME: enforce some maximum cache size (old version had 100K vertices as limit)
QHash<std::string, CGAL_Nef_polyhedron> cache;

void cgalTree(Tree &tree)
{
	assert(tree.root());

	CGALRenderer renderer(cache, tree);
	Traverser render(renderer, *tree.root(), Traverser::PRE_AND_POSTFIX);
	render.execute();
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file.scad>\n", argv[0]);
		exit(1);
	}

	const char *filename = argv[1];

	int rc = 0;

	initialize_builtin_functions();
	initialize_builtin_modules();

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
	root_ctx.functions_p = &builtin_functions;
	root_ctx.modules_p = &builtin_modules;
	root_ctx.set_variable("$fn", Value(0.0));
	root_ctx.set_variable("$fs", Value(1.0));
	root_ctx.set_variable("$fa", Value(12.0));
	root_ctx.set_variable("$t", Value(0.0));

	Value zero3;
	zero3.type = Value::VECTOR;
	zero3.vec.append(new Value(0.0));
	zero3.vec.append(new Value(0.0));
	zero3.vec.append(new Value(0.0));
	root_ctx.set_variable("$vpt", zero3);
	root_ctx.set_variable("$vpr", zero3);


	AbstractModule *root_module;
	ModuleInstantiation root_inst;
	AbstractNode *root_node;

	QFileInfo fileInfo(filename);
	handle_dep(filename);
	FILE *fp = fopen(filename, "rt");
	if (!fp) {
		fprintf(stderr, "Can't open input file `%s'!\n", filename);
		exit(1);
	} else {
		QString text;
		char buffer[513];
		int ret;
		while ((ret = fread(buffer, 1, 512, fp)) > 0) {
			buffer[ret] = 0;
			text += buffer;
		}
		fclose(fp);
		if(!parse(root_module, (text+commandline_commands).toAscii().data(), fileInfo.absolutePath().toLocal8Bit(), false)) {
			delete root_module; // parse failed
			root_module = NULL;
		}
		if (!root_module) {
			exit(1);
		}
	}

	QDir::setCurrent(fileInfo.absolutePath());

	AbstractNode::resetIndexCounter();
	root_node = root_module->evaluate(&root_ctx, &root_inst);

	Tree tree;
	tree.setRoot(root_node);

	cgalTree(tree);

	CGAL_Nef_polyhedron N = cache[tree.getString(*root_node)];

	QDir::setCurrent(original_path.absolutePath());
	QTextStream outstream(stdout);
	export_dxf(&N, outstream, NULL);

	PolySetRenderer::setRenderer(NULL);

	destroy_builtin_functions();
	destroy_builtin_modules();

	return rc;
}
