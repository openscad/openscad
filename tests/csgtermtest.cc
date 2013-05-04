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
#include "PolySetEvaluator.h"
#include "CSGTermEvaluator.h"
#include "openscad.h"
#include "parsersettings.h"
#include "node.h"
#include "module.h"
#include "modcontext.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "Tree.h"
#include "csgterm.h"

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

using std::cout;

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

	fs::path original_path = fs::current_path();

	currentdir = boosty::stringy( fs::current_path() );

	parser_init(boosty::stringy(fs::path(argv[0]).branch_path()));
	add_librarydir(boosty::stringy(fs::path(argv[0]).branch_path() / "../libraries"));

	ModuleContext top_ctx;
	top_ctx.registerBuiltin();

	FileModule *root_module;
	ModuleInstantiation root_inst("group");
	const AbstractNode *root_node;

	root_module = parsefile(filename);
	if (!root_module) {
		exit(1);
	}

	if (fs::path(filename).has_parent_path()) {
		fs::current_path(fs::path(filename).parent_path());
	}

	AbstractNode::resetIndexCounter();
	root_node = root_module->instantiate(&top_ctx, &root_inst);

	Tree tree(root_node);

//	cout << tree.getString(*root_node) << "\n";

	std::vector<shared_ptr<CSGTerm> > highlights;
	std::vector<shared_ptr<CSGTerm> > background;
	PolySetEvaluator psevaluator(tree);
	CSGTermEvaluator evaluator(tree, &psevaluator);
	shared_ptr<CSGTerm> root_term = evaluator.evaluateCSGTerm(*root_node, highlights, background);
	
	// cout << "Stored terms: " << evaluator.stored_term.size() << "\n";
	// for (map<int, class CSGTerm*>::iterator iter = evaluator.stored_term.begin();
	// 		 iter != evaluator.stored_term.end();
	// 		 iter++) {
	// 	cout << iter->first << ":" << (iter->second ? iter->second->label : "NULL") << "\n";
	// }

	// if (evaluator.background) cout << "Background terms: " << evaluator.background->size() << "\n";
	// if (evaluator.highlights) cout << "Highlights terms: " << evaluator.highlights->size() << "\n";

	current_path(original_path);
	std::ofstream outfile;
	outfile.open(outfilename);
	if (root_term) {
		outfile << root_term->dump() << "\n";
	}
	else {
		outfile << "No top-level CSG object\n";
	}
	outfile.close();

	delete root_node;
	delete root_module;

	Builtins::instance(true);

	return rc;
}
