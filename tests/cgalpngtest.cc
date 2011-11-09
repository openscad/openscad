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

#include <QApplication>
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


extern Vector3d getBoundingCenter(BoundingBox bbox);
extern double getBoundingRadius(BoundingBox bbox);


int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <file.scad> <output.png>\n", argv[0]);
		exit(1);
	}

	const char *filename = argv[1];
	const char *outfile = argv[2];

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

	root_module = parsefile(filename);
	if (!root_module) {
		exit(1);
	}

	QFileInfo fileInfo(filename);
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
       try {
                csgInfo.glview = new OffscreenView(512,512);
        } catch (int error) {
                fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i. Exiting.\n", error);
                exit(1);
        }

  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "Unable to init GLEW: %s\n", glewGetErrorString(err));
    exit(1);
  }
#ifdef DEBUG
	std::cout << "GLEW version " << glewGetString(GLEW_VERSION) << "\n";
	std::cout << (const char *)glGetString(GL_RENDERER) << "(" << (const char *)glGetString(GL_VENDOR) << ")\n"
						<< "OpenGL version " << (const char *)glGetString(GL_VERSION) << "\n";
	std::cout  << "Extensions: " << (const char *)glGetString(GL_EXTENSIONS) << "\n";


	if (GLEW_ARB_framebuffer_object) {
		std::cout << "ARB_FBO supported\n";
	}
	if (GLEW_EXT_framebuffer_object) {
		std::cout << "EXT_FBO supported\n";
	}
	if (GLEW_EXT_packed_depth_stencil) {
		std::cout << "EXT_packed_depth_stencil\n";
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

	Vector3d center = getBoundingCenter(bbox);
	double radius = getBoundingRadius(bbox);

	Vector3d cameradir(1, 1, -0.5);
	Vector3d camerapos = center - radius*2*cameradir;
	csgInfo.glview->setCamera(camerapos, center);


	csgInfo.glview->setRenderer(&cgalRenderer);
	csgInfo.glview->paintGL();
	csgInfo.glview->save(outfile);

	Builtins::instance(true);

	return 0;
}
