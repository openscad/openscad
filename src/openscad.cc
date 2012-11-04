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

#include "openscad.h"
#include "MainWindow.h"
#include "node.h"
#include "module.h"
#include "context.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "nodedumper.h"
#include "printutils.h"
#include "handle_dep.h"
#include "parsersettings.h"

#include <string>
#include <vector>
#include <fstream>

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "CGALEvaluator.h"
#include "PolySetCGALEvaluator.h"
#endif

#include <QApplication>
#include <QDir>
#include <sstream>

#ifdef Q_WS_MAC
#include "EventFilter.h"
#include "AppleEvents.h"
#endif

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "boosty.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;

static void help(const char *progname)
{
	fprintf(stderr, "Usage: %s [ -o output_file [ -d deps_file ] ]\\\n"
					"%*s[ -m make_command ] [ -D var=val [..] ] filename\n",
					progname, int(strlen(progname))+8, "");
	exit(1);
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
static void version()
{
	printf("OpenSCAD version %s\n", TOSTRING(OPENSCAD_VERSION));
	exit(1);
}

std::string commandline_commands;
std::string currentdir;
QString examplesdir;

using std::string;
using std::vector;

int main(int argc, char **argv)
{
	int rc = 0;

#ifdef ENABLE_CGAL
	// Causes CGAL errors to abort directly instead of throwing exceptions
	// (which we don't catch). This gives us stack traces without rerunning in gdb.
	CGAL::set_error_behaviour(CGAL::ABORT);
#endif
	Builtins::instance()->initialize();

#ifdef Q_WS_X11
	// see <http://qt.nokia.com/doc/4.5/qapplication.html#QApplication-2>:
	// On X11, the window system is initialized if GUIenabled is true. If GUIenabled
	// is false, the application does not connect to the X server. On Windows and
	// Macintosh, currently the window system is always initialized, regardless of the
	// value of GUIenabled. This may change in future versions of Qt.
	bool useGUI = getenv("DISPLAY") != 0;
#else
	bool useGUI = true;
#endif
	QApplication app(argc, argv, useGUI);
#ifdef Q_WS_MAC
	app.installEventFilter(new EventFilter(&app));
#endif
	fs::path original_path = fs::current_path();

	// set up groups for QSettings
	QCoreApplication::setOrganizationName("OpenSCAD");
	QCoreApplication::setOrganizationDomain("openscad.org");
	QCoreApplication::setApplicationName("OpenSCAD");
	QCoreApplication::setApplicationVersion(TOSTRING(OPENSCAD_VERSION));

	const char *filename = NULL;
	const char *output_file = NULL;
	const char *deps_output_file = NULL;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "help message")
		("version,v", "print the version")
		("o,o", po::value<string>(), "out-file")
		("s,s", po::value<string>(), "stl-file")
		("x,x", po::value<string>(), "dxf-file")
		("d,d", po::value<string>(), "deps-file")
		("m,m", po::value<string>(), "makefile")
		("D,D", po::value<vector<string> >(), "var=val");

	po::options_description hidden("Hidden options");
	hidden.add_options()
		("input-file", po::value< vector<string> >(), "input file");

	po::positional_options_description p;
	p.add("input-file", -1);

	po::options_description all_options;
	all_options.add(desc).add(hidden);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(all_options).positional(p).run(), vm);
	}
	catch(const std::exception &e) { // Catches e.g. unknown options
		fprintf(stderr, "%s\n", e.what());
		help(argv[0]);
	}

	if (vm.count("help")) help(argv[0]);
	if (vm.count("version")) version();

	if (vm.count("o")) {
		// FIXME: Allow for multiple output files?
		if (output_file) help(argv[0]);
		output_file = vm["o"].as<string>().c_str();
	}
	if (vm.count("s")) {
		fprintf(stderr, "DEPRECATED: The -s option is deprecated. Use -o instead.\n");
		if (output_file) help(argv[0]);
		output_file = vm["s"].as<string>().c_str();
	}
	if (vm.count("x")) { 
		fprintf(stderr, "DEPRECATED: The -x option is deprecated. Use -o instead.\n");
		if (output_file) help(argv[0]);
		output_file = vm["x"].as<string>().c_str();
	}
	if (vm.count("d")) {
		if (deps_output_file)
			help(argv[0]);
		deps_output_file = vm["d"].as<string>().c_str();
	}
	if (vm.count("m")) {
		if (make_command)
			help(argv[0]);
		make_command = vm["m"].as<string>().c_str();
	}

	if (vm.count("D")) {
		BOOST_FOREACH(const string &cmd, vm["D"].as<vector<string> >()) {
			commandline_commands += cmd;
			commandline_commands += ";\n";
		}
	}

	if (vm.count("input-file")) {
		filename = vm["input-file"].as< vector<string> >().begin()->c_str();
	}

#ifndef ENABLE_MDI
	if (vm.count("input-file") > 1) {
		help(argv[0]);
	}
#endif

	currentdir = boosty::stringy(fs::current_path());

	QDir exdir(QApplication::instance()->applicationDirPath());
#ifdef Q_WS_MAC
	exdir.cd("../Resources"); // Examples can be bundled
	if (!exdir.exists("examples")) exdir.cd("../../..");
#elif defined(Q_OS_UNIX)
	if (exdir.cd("../share/openscad/examples")) {
		examplesdir = exdir.path();
	} else
		if (exdir.cd("../../share/openscad/examples")) {
			examplesdir = exdir.path();
		} else
			if (exdir.cd("../../examples")) {
				examplesdir = exdir.path();
			} else
#endif
				if (exdir.cd("examples")) {
					examplesdir = exdir.path();
				}

	parser_init(QApplication::instance()->applicationDirPath().toStdString());

	// Initialize global visitors
	NodeCache nodecache;
	NodeDumper dumper(nodecache);
	Tree tree;
#ifdef ENABLE_CGAL
	CGALEvaluator cgalevaluator(tree);
	PolySetCGALEvaluator psevaluator(cgalevaluator);
#endif

	if (output_file)
	{
		const char *stl_output_file = NULL;
		const char *off_output_file = NULL;
		const char *dxf_output_file = NULL;
		const char *csg_output_file = NULL;

		QString suffix = QFileInfo(output_file).suffix().toLower();
		if (suffix == "stl") stl_output_file = output_file;
		else if (suffix == "off") off_output_file = output_file;
		else if (suffix == "dxf") dxf_output_file = output_file;
		else if (suffix == "csg") csg_output_file = output_file;
		else {
			fprintf(stderr, "Unknown suffix for output file %s\n", output_file);
			exit(1);
		}

		if (!filename) help(argv[0]);

#ifdef ENABLE_CGAL
		Context root_ctx;
		register_builtin(root_ctx);

		Module *root_module;
		ModuleInstantiation root_inst;
		AbstractNode *root_node;

		handle_dep(filename);
		
		std::ifstream ifs(filename);
		if (!ifs.is_open()) {
			fprintf(stderr, "Can't open input file '%s'!\n", filename);
			exit(1);
		}
		std::string text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		text += "\n" + commandline_commands;
		fs::path abspath = boosty::absolute(filename);
		std::string parentpath = boosty::stringy(abspath.parent_path());
		root_module = parse(text.c_str(), parentpath.c_str(), false);
		if (!root_module) exit(1);
		root_module->handleDependencies();
		
		fs::path fpath = boosty::absolute(fs::path(filename));
		fs::path fparent = fpath.parent_path();
		fs::current_path(fparent);

		AbstractNode::resetIndexCounter();
		root_node = root_module->evaluate(&root_ctx, &root_inst);
		tree.setRoot(root_node);

		if (csg_output_file) {
			fs::current_path(original_path);
			std::ofstream fstream(csg_output_file);
			if (!fstream.is_open()) {
				PRINTB("Can't open file \"%s\" for export", csg_output_file);
			}
			else {
				fs::current_path(fparent); // Force exported filenames to be relative to document path
				fstream << tree.getString(*root_node) << "\n";
				fstream.close();
			}
		}
		else {
			CGAL_Nef_polyhedron root_N = cgalevaluator.evaluateCGALMesh(*tree.root());
			
			fs::current_path(original_path);
			
			if (deps_output_file) {
				if (!write_deps(deps_output_file, 
												stl_output_file ? stl_output_file : off_output_file)) {
					exit(1);
				}
			}

			if (stl_output_file) {
				if (root_N.dim != 3) {
					fprintf(stderr, "Current top level object is not a 3D object.\n");
					exit(1);
				}
				if (!root_N.p3->is_simple()) {
					fprintf(stderr, "Object isn't a valid 2-manifold! Modify your design.\n");
					exit(1);
				}
				std::ofstream fstream(stl_output_file);
				if (!fstream.is_open()) {
					PRINTB("Can't open file \"%s\" for export", stl_output_file);
				}
				else {
					export_stl(&root_N, fstream);
					fstream.close();
				}
			}
			
			if (off_output_file) {
				if (root_N.dim != 3) {
					fprintf(stderr, "Current top level object is not a 3D object.\n");
					exit(1);
				}
				if (!root_N.p3->is_simple()) {
					fprintf(stderr, "Object isn't a valid 2-manifold! Modify your design.\n");
					exit(1);
				}
				std::ofstream fstream(off_output_file);
				if (!fstream.is_open()) {
					PRINTB("Can't open file \"%s\" for export", off_output_file);
				}
				else {
					export_off(&root_N, fstream);
					fstream.close();
				}
			}
			
			if (dxf_output_file) {
				if (root_N.dim != 2) {
					fprintf(stderr, "Current top level object is not a 2D object.\n");
					exit(1);
				}
				std::ofstream fstream(dxf_output_file);
				if (!fstream.is_open()) {
					PRINTB("Can't open file \"%s\" for export", dxf_output_file);
				}
				else {
					export_dxf(&root_N, fstream);
					fstream.close();
				}
			}
		}
		delete root_node;
#else
		fprintf(stderr, "OpenSCAD has been compiled without CGAL support!\n");
		exit(1);
#endif
	}
	else if (useGUI)
	{
#ifdef Q_WS_MAC
		installAppleEventHandlers();
#endif		

		QString qfilename;
		if (filename) qfilename = QString::fromStdString(boosty::stringy(boosty::absolute(filename)));

#if 0 /*** disabled by clifford wolf: adds rendering artefacts with OpenCSG ***/
		// turn on anti-aliasing
		QGLFormat f;
		f.setSampleBuffers(true);
		f.setSamples(4);
		QGLFormat::setDefaultFormat(f);
#endif
#ifdef ENABLE_MDI
		new MainWindow(qfilename);
		vector<string> inputFiles;
		if (vm.count("input-file")) {
			inputFiles = vm["input-file"].as<vector<string> >();
			for (vector<string>::const_iterator infile = inputFiles.begin()+1; infile != inputFiles.end(); infile++) {
				new MainWindow(QString::fromStdString(boosty::stringy((original_path / *infile))));
			}
		}
		app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
#else
		MainWindow *m = new MainWindow(qfilename);
		app.connect(m, SIGNAL(destroyed()), &app, SLOT(quit()));
#endif
		rc = app.exec();
	}
	else
	{
		fprintf(stderr, "Requested GUI mode but can't open display!\n");
		exit(1);
	}

	Builtins::instance(true);

	return rc;
}

