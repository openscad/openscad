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
#include "polyset.h"
#include "context.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "Tree.h"
#include "CGAL_Nef_polyhedron.h"
#include "CGALEvaluator.h"
#include "PolySetCGALEvaluator.h"
#include "CGALRenderer.h"
#include "CGAL_renderer.h"
#include "cgal.h"
#include "OffscreenView.h"
#include "handle_dep.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSet>
#include <QTextStream>
#include <getopt.h>
#include <iostream>
#include <assert.h>
#include <sstream>

std::string commandline_commands;
QString currentdir;
QString examplesdir;
QString librarydir;

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

struct CsgInfo
{
	OffscreenView *glview;
};

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file.scad>\n", argv[0]);
		exit(1);
	}

	const char *filename = argv[1];

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
	zero3.append(new Value(0.0));
	zero3.append(new Value(0.0));
	zero3.append(new Value(0.0));
	root_ctx.set_variable("$vpt", zero3);
	root_ctx.set_variable("$vpr", zero3);


	AbstractModule *root_module;
	ModuleInstantiation root_inst;

	QFileInfo fileInfo(filename);
	handle_dep(filename);
	FILE *fp = fopen(filename, "rt");
	if (!fp) {
		fprintf(stderr, "Can't open input file `%s'!\n", filename);
		exit(1);
	} else {
		std::stringstream text;
		char buffer[513];
		int ret;
		while ((ret = fread(buffer, 1, 512, fp)) > 0) {
			buffer[ret] = 0;
			text << buffer;
		}
		fclose(fp);
		text << commandline_commands;
		root_module = parse(text.str().c_str(), fileInfo.absolutePath().toLocal8Bit(), false);
		if (!root_module) {
			exit(1);
		}
	}

	QDir::setCurrent(fileInfo.absolutePath());

	AbstractNode::resetIndexCounter();
	AbstractNode *absolute_root_node = root_module->evaluate(&root_ctx, &root_inst);
	AbstractNode *root_node;
	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node))) root_node = absolute_root_node;

	Tree tree(root_node);

	CsgInfo csgInfo;
	CGALEvaluator cgalevaluator(tree);
 	PolySetCGALEvaluator psevaluator(cgalevaluator);

	CGAL_Nef_polyhedron N = cgalevaluator.evaluateCGALMesh(*root_node);

	QDir::setCurrent(original_path.absolutePath());

// match with csgtest ends
	csgInfo.glview = new OffscreenView(512,512);

  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "Unable to init GLEW: %s\n", glewGetErrorString(err));
    exit(1);
  }
#ifdef DEBUG
	cout << "GLEW version " << glewGetString(GLEW_VERSION) << "\n";
	cout << (const char *)glGetString(GL_RENDERER) << "(" << (const char *)glGetString(GL_VENDOR) << ")\n"
			 << "OpenGL version " << (const char *)glGetString(GL_VERSION) << "\n";
	cout  << "Extensions: " << (const char *)glGetString(GL_EXTENSIONS) << "\n";


	if (GLEW_ARB_framebuffer_object) {
		cout << "ARB_FBO supported\n";
	}
	if (GLEW_EXT_framebuffer_object) {
		cout << "EXT_FBO supported\n";
	}
	if (GLEW_EXT_packed_depth_stencil) {
		cout << "EXT_packed_depth_stencil\n";
	}
#endif

	CGALRenderer cgalRenderer(N);

	BoundingBox bbox;
	if (cgalRenderer.polyhedron) {
		CGAL::Bbox_3 cgalbbox = cgalRenderer.polyhedron->bbox();
		bbox = BoundingBox(Vector3d(cgalbbox.xmin(), cgalbbox.ymin(), cgalbbox.zmin()),
											 Vector3d(cgalbbox.xmax(), cgalbbox.ymax(), cgalbbox.zmax()));
	}
	else if (cgalRenderer.polyset) {
		bbox = cgalRenderer.polyset->getBoundingBox();
	}
	Vector3d center = (bbox.min() + bbox.max()) / 2;
	double radius = (bbox.max() - bbox.min()).norm() / 2;


	Vector3d cameradir(1, 1, -0.5);
	Vector3d camerapos = center - radius*2*cameradir;
	csgInfo.glview->setCamera(camerapos, center);


	csgInfo.glview->setRenderer(&cgalRenderer);
	csgInfo.glview->paintGL();
	csgInfo.glview->save("/dev/stdout");

	destroy_builtin_functions();
	destroy_builtin_modules();

	return 0;
}
