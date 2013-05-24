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
#include "polyset.h"
#include "modcontext.h"
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

using std::string;

void cgalTree(Tree &tree)
{
	assert(tree.root());

	CGALEvaluator evaluator(tree);
	Traverser evaluate(evaluator, *tree.root(), Traverser::PRE_AND_POSTFIX);
	evaluate.execute();
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

#ifdef ENABLE_CGAL
	// Causes CGAL errors to abort directly instead of throwing exceptions
	// (which we don't catch). This gives us stack traces without rerunning in gdb.
	CGAL::set_error_behaviour(CGAL::ABORT);
#endif
	Builtins::instance()->initialize();

	fs::path original_path = fs::current_path();

	currentdir = boosty::stringy( fs::current_path() );

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

	fs::path fpath = boosty::absolute(fs::path(filename));
	fs::path fparent = fpath.parent_path();
	fs::current_path(fparent);
	top_ctx.setDocumentPath(fparent.string());

	AbstractNode::resetIndexCounter();
	AbstractNode *absolute_root_node = root_module->instantiate(&top_ctx, &root_inst);
	AbstractNode *root_node;
	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node))) root_node = absolute_root_node;

	Tree tree(root_node);

	CsgInfo csgInfo;
	CGALEvaluator cgalevaluator(tree);
 	PolySetCGALEvaluator psevaluator(cgalevaluator);

	CGAL_Nef_polyhedron N = cgalevaluator.evaluateCGALMesh(*root_node);

	current_path(original_path);

// match with csgtest ends
       try {
                csgInfo.glview = new OffscreenView(512,512);
        } catch (int error) {
                fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i. Exiting.\n", error);
                exit(1);
        }

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
	
	Camera cam(Camera::VECTOR);
	cam.center = getBoundingCenter(bbox);
	double radius = getBoundingRadius(bbox);
	
	Vector3d cameradir(1, 1, -0.5);
	cam.eye = cam.center - radius*2*cameradir;
	csgInfo.glview->setCamera( cam );

	csgInfo.glview->setRenderer(&cgalRenderer);
	csgInfo.glview->paintGL();
	csgInfo.glview->save(outfile);
	
	delete root_node;
	delete root_module;

	Builtins::instance(true);

	return 0;
}
