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
#include "printutils.h"
#include "parsersettings.h"
#include "node.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "modcontext.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "Tree.h"
#include "CGAL_Nef_polyhedron.h"
#include "GeometryEvaluator.h"
#include "CGALCache.h"
#include "stackcheck.h"

#ifndef _MSC_VER
#include <getopt.h>
#endif
#include <iostream>
#include <assert.h>
#include <sstream>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include "PlatformUtils.h"

std::string commandline_commands;
std::string currentdir;

using std::string;

po::variables_map parse_options(int argc, char *argv[])
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "help message")
		("cgalcachesize", po::value<size_t>(), "Set CGAL cache size in bytes");
	
	po::options_description hidden("Hidden options");
	hidden.add_options()
		("input-file", po::value<string>(), "input file")
		("output-file", po::value<string>(), "output file");
	
	po::positional_options_description p;
	p.add("input-file", 1).add("output-file", 1);
	
	po::options_description all_options;
	all_options.add(desc).add(hidden);
	
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(all_options).positional(p).run(), vm);
	po::notify(vm);
	
	return vm;
}

int main(int argc, char **argv)
{
	const char *filename, *outfilename = NULL;
	size_t cgalcachesize = 1*1024*1024;
	StackCheck::inst()->init();

	po::variables_map vm;
	try {
		vm = parse_options(argc, argv);
	} catch ( po::error e ) {
		std::cerr << "error parsing options: " << e.what() << "\n";
	}
	if (vm.count("cgalcachesize")) {
		cgalcachesize = vm["cgalcachesize"].as<size_t>();
	}
	if (vm.count("input-file")) {
		filename = vm["input-file"].as<string>().c_str();
	}
	if (vm.count("output-file")) {
		outfilename = vm["output-file"].as<string>().c_str();
	}

	if ((!filename || !outfilename)) {
		std::cerr << "Usage: " << argv[0] << " <file.scad> <output.txt>\n";
		exit(1);
	}

	CGALCache::instance()->setMaxSize(cgalcachesize);
	
	Builtins::instance()->initialize();

	fs::path original_path = fs::current_path();

	currentdir = fs::current_path().generic_string();

	PlatformUtils::registerApplicationPath(fs::path(argv[0]).branch_path().generic_string());
	parser_init();

	ModuleContext top_ctx;
	top_ctx.registerBuiltin();

	FileModule *root_module;
	ModuleInstantiation root_inst("group");

	root_module = parsefile(filename);
	if (!root_module) {
		exit(1);
	}

	if (fs::path(filename).has_parent_path()) {
		fs::current_path(fs::path(filename).parent_path());
	}

	AbstractNode::resetIndexCounter();
	AbstractNode *absolute_root_node = root_module->instantiate(&top_ctx, &root_inst);
	AbstractNode *root_node;
	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node))) root_node = absolute_root_node;

	Tree tree(root_node);

	GeometryEvaluator geomevaluator(tree);

	print_messages_push();

	std::cout << "First evaluation:\n";
	shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*root_node, true);
	std::cout << "Second evaluation:\n";
	shared_ptr<const Geometry> geom2 = geomevaluator.evaluateGeometry(*root_node, true);
	// FIXME:
	// Evaluate again to make cache kick in
	// Record printed output and compare it
	// Compare the polyhedrons
  // Record cache statistics?

	print_messages_pop();
	current_path(original_path);

	Builtins::instance(true);

	return 0;
}
