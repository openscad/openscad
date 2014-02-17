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
#include "node.h"
#include "module.h"
#include "modcontext.h"
#include "value.h"
#include "export.h"
#include "builtin.h"
#include "printutils.h"
#include "handle_dep.h"
#include "feature.h"
#include "parsersettings.h"
#include "rendersettings.h"
#include "PlatformUtils.h"
#include "nodedumper.h"
#include "CocoaUtils.h"

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "CGALEvaluator.h"
#include "PolySetCGALEvaluator.h"
#endif

#include "csgterm.h"
#include "CSGTermEvaluator.h"
#include "CsgInfo.h"

#include <sstream>

#ifdef __APPLE__
#include "AppleEvents.h"
#ifdef OPENSCAD_DEPLOY
  #include "SparkleAutoUpdater.h"
#endif
#endif

#include "Camera.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include "boosty.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace Render { enum type { CGAL, OPENCSG, THROWNTOGETHER }; };

using std::function;
using std::map;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;
using boost::lexical_cast;
using boost::is_any_of;
using boost::optional;

string commandline_commands;
string currentdir;

static void help(const char *progname, vector<po::options_description> options)
{
  std::ostringstream ss;
  ss << "Usage: " << progname << " [options] [action]\n";
  for (auto &opt : options)
    ss << "\n" << opt ;
  PRINT(ss.str());
  exit(1);
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
static void version()
{
	PRINTB("OpenSCAD version %s\n", TOSTRING(OPENSCAD_VERSION));
	exit(1);
}

static void info()
{
	std::cout << PlatformUtils::info() << "\n\n";

	CsgInfo csgInfo = CsgInfo();
	try {
		csgInfo.glview = new OffscreenView(512,512);
	} catch (int error) {
		PRINTB("Can't create OpenGL OffscreenView. Code: %i. Exiting.\n", error);
		exit(1);
	}

	std::cout << csgInfo.glview->getRendererInfo() << "\n";

	exit(0);
}

Camera get_camera( po::variables_map vm )
{
	Camera camera;

	if (vm.count("camera")) {
		vector<string> strs;
		vector<double> cam_parameters;
		split(strs, vm["camera"].as<string>(), is_any_of(","));
		if ( strs.size() == 6 || strs.size() == 7 ) {
			BOOST_FOREACH(string &s, strs)
				cam_parameters.push_back(lexical_cast<double>(s));
			camera.setup( cam_parameters );
		} else {
			PRINT("Camera setup requires either 7 numbers for Gimbal Camera\n");
			PRINT("or 6 numbers for Vector Camera\n");
			exit(1);
		}
	}

	if (camera.type == Camera::GIMBAL) {
		camera.gimbalDefaultTranslate();
	}

	if (vm.count("projection")) {
		string proj = vm["projection"].as<string>();
		if (proj=="o" || proj=="ortho" || proj=="orthogonal")
			camera.projection = Camera::ORTHOGONAL;
		else if (proj=="p" || proj=="perspective")
			camera.projection = Camera::PERSPECTIVE;
		else {
			PRINT("projection needs to be 'o' or 'p' for ortho or perspective\n");
			exit(1);
		}
	}

	int w = RenderSettings::inst()->img_width;
	int h = RenderSettings::inst()->img_height;
	if (vm.count("imgsize")) {
		vector<string> strs;
		split(strs, vm["imgsize"].as<string>(), is_any_of(","));
		if ( strs.size() != 2 ) {
			PRINT("Need 2 numbers for imgsize\n");
			exit(1);
		} else {
			w = lexical_cast<int>( strs[0] );
			h = lexical_cast<int>( strs[1] );
		}
	}
	camera.pixel_width = w;
	camera.pixel_height = h;

	return camera;
}

static bool assert_root_3d_simple(CGAL_Nef_polyhedron &nef) {
	if (nef.dim != 3) {
		PRINT("Current top level object is not a 3D object.\n");
		return false;
	}
	if (!nef.p3->is_simple()) {
		PRINT("Object isn't a valid 2-manifold! Modify your design.\n");
		return false;
	}
	return true;
}

static bool assert_root_2d(CGAL_Nef_polyhedron &nef) {
	if (nef.dim != 2) {
		PRINT("Current top level object is not a 2D object.\n");
		return false;
	}
	return true;
};

int cmdline(optional<string> action, optional<string> output_file,
	    optional<string> deps_output_file,
	    const string &filename,
	    Camera &camera, Render::type renderer,
	    const fs::path &original_path, string application_name)
{
	CGAL_Nef_polyhedron root_N;
	Tree tree;
	unique_ptr<CGALEvaluator> cgalevaluator;
	fs::path fparent;
	FileModule *root_module;
	AbstractNode *root_node;
	std::ostream *output_stream;

	// list of actions to be performed, indexed by filename suffix
	map<string, function<int(void)>> actions{
#ifdef ENABLE_CGAL
		{"stl", [&] {
			root_N = cgalevaluator->evaluateCGALMesh(*tree.root());
			if (!assert_root_3d_simple(root_N)) return 1;
			export_stl(&root_N, *output_stream);
			return 0; }},
		{"off", [&] {
			root_N = cgalevaluator->evaluateCGALMesh(*tree.root());
			if (!assert_root_3d_simple(root_N)) return 1;
			export_off(&root_N, *output_stream);
			return 0; }},
		{"dxf", [&] {
			root_N = cgalevaluator->evaluateCGALMesh(*tree.root());
			if (!assert_root_2d(root_N)) return 1;
			export_dxf(&root_N, *output_stream);
			return 0; }},
		{"png", [&] {
			switch (renderer) {
			case Render::CGAL:
				root_N = cgalevaluator->evaluateCGALMesh(*tree.root());
				export_png_with_cgal(&root_N, camera, *output_stream);
				break;
			case Render::THROWNTOGETHER:
				export_png_with_throwntogether(tree, camera, *output_stream);
				break;
			case Render::OPENCSG:
				export_png_with_opencsg(tree, camera, *output_stream);
				break;
			}
			return 0; }},
		{"echo", [&] {
			if (renderer == Render::CGAL)
				root_N = cgalevaluator->evaluateCGALMesh(*tree.root());
			return 0; }},
		{"term", [&] {
			// TODO: check wether CWD is correct at this point
			PolySetCGALEvaluator psevaluator(*cgalevaluator);
			CSGTermEvaluator csgRenderer(tree, &psevaluator);
			vector<shared_ptr<CSGTerm> > highlight_terms, background_terms;
			shared_ptr<CSGTerm> root_raw_term = csgRenderer.evaluateCSGTerm(*root_node, highlight_terms, background_terms);

			if (!root_raw_term) {
				*output_stream << "No top-level CSG object\n";
			} else {
				*output_stream << root_raw_term->dump() << "\n";
			}
			return 0; }},
#endif
		{"csg", [&] {
			fs::current_path(fparent); // Force exported filenames to be relative to document path
			*output_stream << tree.getString(*root_node) << "\n";
			return 0; }},

		{"ast", [&] {
			fs::current_path(fparent); // Force exported filenames to be relative to document path
			*output_stream << root_module->dump("", "") << "\n";
			return 0; }}
	};

	// Set action and output filename; both are optional
	if (!action && (!output_file || (*output_file == "-")))
		action = "stl";
	if (!action) {
		// Guess action from filename suffix
		action = boosty::extension_str(*output_file);
		boost::algorithm::to_lower(*action);
		if (action->length() > 0)
			action = action->substr(1); // remove leading dot
		if (!actions.count(*action)) {
			PRINTB("Unknown suffix for output file %s\n", *output_file);
			return 1;
		}
	}
	if (!output_file)
		output_file = filename + "." + *action;

	// Open output stream. If it refers to a file, use a temporary
	// file first. Filename "-" refers to standard output
	std::ofstream output_file_stream;
	optional<string> temp_output_file;
	if (*output_file == "-") {
		output_stream = &(std::cout);
	} else {
		temp_output_file = *output_file + "~";
		output_file_stream.open(*temp_output_file, (*action == "png") ? std::ios::binary : std::ios::out);
		if (!output_file_stream.is_open()) {
			PRINTB("Can't open file \"%s\" for export", *temp_output_file);
			return 1;
		}
		output_stream = &output_file_stream;
	}

	// Open an echo stream early
	if (*action == "echo")
		set_output_handler([&](string msg) { *output_stream << msg << "\n"; });

	// Init parser, top_con
	const string application_path = boosty::stringy(boosty::absolute(boost::filesystem::path(application_name).parent_path()));
	parser_init(application_path);
#ifdef ENABLE_CGAL
	cgalevaluator.reset(new CGALEvaluator(tree));
#endif

	// Top context - this context only holds builtins
	ModuleContext top_ctx;
	top_ctx.registerBuiltin();
#if 0 && DEBUG
	top_ctx.dump(NULL, NULL);
#endif

	ModuleInstantiation root_inst("group");
	AbstractNode *absolute_root_node;

	// Open the root source document. Either from file or stdin.
	string text, parentpath;
	if (filename != "-") {
	  handle_dep(filename);

	  std::ifstream ifs(filename.c_str());
	  if (!ifs.is_open()) {
	    PRINTB("Can't open input file '%s'!\n", filename.c_str());
	    return 1;
	  }
	  text = string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	  fs::path abspath = boosty::absolute(filename);
	  parentpath = boosty::stringy(abspath.parent_path());
	}else{
	  text = string((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
	  parentpath = boosty::stringy(original_path);
	}
	text += "\n" + commandline_commands;

	root_module = parse(text.c_str(), parentpath.c_str(), false);
	if (!root_module) {
		PRINTB("Can't parse file '%s'!\n", filename.c_str());
		return 1;
	}
	root_module->handleDependencies();

	fs::path fpath = boosty::absolute(fs::path(filename));
	fparent = fpath.parent_path();
	fs::current_path(fparent);
	top_ctx.setDocumentPath(fparent.string());

	AbstractNode::resetIndexCounter();
	absolute_root_node = root_module->instantiate(&top_ctx, &root_inst, NULL);

	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node)))
		root_node = absolute_root_node;

	tree.setRoot(root_node);

	// Write dependencies if required
	if (deps_output_file) {
		if (!set<string>{"stl", "off", "dxf", "png"}.count(*action)) {
			PRINTB("Output file: %s\n", *output_file);
			PRINT("Sorry, don't know how to write deps for that file type. Exiting\n");
			return 1;
		}
		if (!write_deps(*deps_output_file, *output_file)) {
			PRINT("error writing deps");
			return 1;
		}
	}

	// Call the intended action
	auto ret = actions[*action]();

	// Commit the file if succesfull, delete it otherwise.
	if (temp_output_file) {
		if (!ret) {
			// Success
			if (rename(temp_output_file->c_str(), output_file->c_str())) {
				PRINTB("Can't rename \"%s\" to \"%s\"", *temp_output_file % *output_file);
				return 1;
			}
		}else{
			// Failure
			if (remove(temp_output_file->c_str())) {
				PRINTB("Can't remove \"%s\"", *temp_output_file);
				return 1;
			}
		}
	}

	// Clean up
	delete root_node;
	return ret;
}

#ifdef OPENSCAD_TESTING
#undef OPENSCAD_QTGUI
#else
#define OPENSCAD_QTGUI 1
#endif


#ifdef OPENSCAD_QTGUI
#include "MainWindow.h"
  #ifdef __APPLE__
  #include "EventFilter.h"
  #endif
#include <QApplication>
#include <QString>
#include <QDir>
#include <QFileInfo>

// Only if "fileName" is not absolute, prepend the "absoluteBase".
static QString assemblePath(const fs::path& absoluteBaseDir,
                            const string& fileName) {
  if (fileName.empty()) return "";
  QString qsDir( boosty::stringy( absoluteBaseDir ).c_str() );
  QString qsFile( fileName.c_str() );
  QFileInfo info( qsDir, qsFile ); // if qsfile is absolute, dir is ignored.
  return info.absoluteFilePath();
}

bool QtUseGUI()
{
#ifdef Q_OS_X11
	// see <http://qt.nokia.com/doc/4.5/qapplication.html#QApplication-2>:
	// On X11, the window system is initialized if GUIenabled is true. If GUIenabled
	// is false, the application does not connect to the X server. On Windows and
	// Macintosh, currently the window system is always initialized, regardless of the
	// value of GUIenabled. This may change in future versions of Qt.
	bool useGUI = getenv("DISPLAY") != 0;
#else
	bool useGUI = true;
#endif
	return useGUI;
}

int gui(vector<string> &inputFiles, const fs::path &original_path, int argc, char ** argv)
{
#ifdef Q_OS_MACX
    if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_8) {
			// fix Mac OS X 10.9 (mavericks) font issue
			// https://bugreports.qt-project.org/browse/QTBUG-32789
			QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif
	QApplication app(argc, argv, true); //useGUI);
#ifdef Q_OS_MAC
	app.installEventFilter(new EventFilter(&app));
#endif
	// set up groups for QSettings
	QCoreApplication::setOrganizationName("OpenSCAD");
	QCoreApplication::setOrganizationDomain("openscad.org");
	QCoreApplication::setApplicationName("OpenSCAD");
	QCoreApplication::setApplicationVersion(TOSTRING(OPENSCAD_VERSION));
	
	const QString &app_path = app.applicationDirPath();

	QDir exdir(app_path);
	QString qexamplesdir;
#ifdef Q_OS_MAC
	exdir.cd("../Resources"); // Examples can be bundled
	if (!exdir.exists("examples")) exdir.cd("../../..");
#elif defined(Q_OS_UNIX)
	if (exdir.cd("../share/openscad/examples")) {
		qexamplesdir = exdir.path();
	} else
		if (exdir.cd("../../share/openscad/examples")) {
			qexamplesdir = exdir.path();
		} else
			if (exdir.cd("../../examples")) {
				qexamplesdir = exdir.path();
			} else
#endif
				if (exdir.cd("examples")) {
					qexamplesdir = exdir.path();
				}
	MainWindow::setExamplesDir(qexamplesdir);
  parser_init(app_path.toLocal8Bit().constData());

#ifdef Q_OS_MAC
	installAppleEventHandlers();
#endif

#if defined(OPENSCAD_DEPLOY) && defined(Q_OS_MAC)
	AutoUpdater *updater = new SparkleAutoUpdater;
	AutoUpdater::setUpdater(updater);
	if (updater->automaticallyChecksForUpdates()) updater->checkForUpdates();
#endif

#if 0 /*** disabled by clifford wolf: adds rendering artefacts with OpenCSG ***/
	// turn on anti-aliasing
	QGLFormat f;
	f.setSampleBuffers(true);
	f.setSamples(4);
	QGLFormat::setDefaultFormat(f);
#endif
	if (!inputFiles.size()) inputFiles.push_back("");
#ifdef ENABLE_MDI
	BOOST_FOREACH(const string &infile, inputFiles) {
               new MainWindow(assemblePath(original_path, infile));
	}
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
#else
	MainWindow *m = new MainWindow(assemblePath(original_path, inputFiles[0]));
	app.connect(m, SIGNAL(destroyed()), &app, SLOT(quit()));
#endif
	return app.exec();
}
#else // OPENSCAD_QTGUI
bool QtUseGUI() { return false; }
int gui(const vector<string> &inputFiles, const fs::path &original_path, int argc, char ** argv)
{
	PRINT("Error: compiled without QT, but trying to run GUI\n");
	return 1;
}
#endif // OPENSCAD_QTGUI

int main(int argc, char **argv)
{
#ifdef Q_OS_MAC
	set_output_handler(CocoaUtils::nslog);
#endif
#ifdef ENABLE_CGAL
	// Causes CGAL errors to abort directly instead of throwing exceptions
	// (which we don't catch). This gives us stack traces without rerunning in gdb.
	CGAL::set_error_behaviour(CGAL::ABORT);
#endif
	Builtins::instance()->initialize();

	fs::path original_path = fs::current_path();

	optional<string> output_file, deps_output_file;

	po::options_description opt_actions("Actions (pick none to start the gui)");
	opt_actions.add_options()
	  ("o,o", po::value<string>(), "output file; \"-\" writes to standart output; file extensions determines action unless specified by -a:\n"
	                               "  .stl .off .dxf .csg - export geometry\n"
	                               "  .png - render image\n"
	                               "  .ast - export abstract syntax tree"
	                               // TODO ".term"
                                       // TODO ".echo"
	   )
	  ("action,a", po::value<string>(), "overide action implied by -o")
	  ("help,h", "print this help message")
	  ("info", "print information about the building process")
	  ("version,v", "print the version");

	po::options_description opt_options("Options");
	opt_options.add_options()
	  ("render", "if exporting a png image, do a full CGAL render")
	  ("preview", po::value<string>(), "if exporting a png image, do an OpenCSG(default) or ThrownTogether preview")
	  ("csglimit", po::value<unsigned int>(), "if exporting a png image, stop rendering at the given number of CSG elements")
	  ("camera", po::value<string>(), "parameters for camera when exporting png; one of:\ntranslatex,y,z,rotx,y,z,dist\neyex,y,z,centerx,y,z")
	  ("imgsize", po::value<string>(), "width,height for exporting png")
	  ("projection", po::value<string>(), "(o)rtho or (p)erspective when exporting png")
	  ("m,m", po::value<string>(), "make command")
	  ("d,d", po::value<string>(), "filename to write the dependencies to (in conjunction with -m)")
	  ("D,D", po::value<vector<string> >(), "var=val to override variables")
	  ("enable", po::value<vector<string> >(), "enable experimental features; can be used several times to enable more than one feature");

	po::options_description opt_hidden("Hidden options");
	opt_hidden.add_options()
	  ("input-file", po::value< vector<string> >(), "input file")
	  ("s,s", po::value<string>(), "stl-file")
	  ("x,x", po::value<string>(), "dxf-file");

	po::positional_options_description opt_positional;
	opt_positional.add("input-file", -1);

	po::options_description all_options;
	for (auto &opt : {opt_actions, opt_options, opt_hidden})
	  all_options.add(opt);

	auto help = [&]{ ::help(argv[0], {opt_actions, opt_options}); };

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(all_options).allow_unregistered().positional(opt_positional).run(), vm);
	}
	catch(const std::exception &e) { // Catches e.g. unknown options
		PRINTB("%s\n", e.what());
		help();
	}

	if (vm.count("help")) help();
	if (vm.count("version")) version();
	if (vm.count("info")) info();

	Render::type renderer = Render::OPENCSG;
	if (vm.count("render"))
		renderer = Render::CGAL;
	if (vm.count("preview"))
		if (vm["preview"].as<string>() == "throwntogether")
			renderer = Render::THROWNTOGETHER;

	if (vm.count("csglimit")) {
		RenderSettings::inst()->openCSGTermLimit = vm["csglimit"].as<unsigned int>();
	}

	// lambda to return an optional<string> from an optional option
	auto optstr = [&](string option_name) {
		if (vm.count(option_name)) {
			return optional<string>(vm[option_name].as<string>());
		}else{
			return optional<string>();
		}
	};

	for (auto &option_name : vector<string>{"o", "s", "x"}) {
		if (!vm.count(option_name)) continue;
		// FIXME: Allow for multiple output files?
		if (output_file)
			help();
		if (option_name != "o")
			PRINTB("DEPRECATED: The -% option is deprecated. Use -o instead.\n", option_name);

		output_file = optstr(option_name);
	}
	make_command = optstr("m");
	if (vm.count("D"))
		for (auto &cmd : vm["D"].as<vector<string>>())
			commandline_commands += cmd + ";\n";
	if (vm.count("D"))
		for (auto &feature : vm["enable"].as<vector<string>>())
			Feature::enable_feature(feature);

	vector<string> inputFiles;
	if (vm.count("input-file"))	{
		inputFiles = vm["input-file"].as<vector<string> >();
	}
#ifndef ENABLE_MDI
	if (inputFiles.size() > 1) {
		help();
	}
#endif

	currentdir = boosty::stringy(fs::current_path());

	Camera camera = get_camera( vm );

	// Initialize global visitors
	NodeCache nodecache;
	NodeDumper dumper(nodecache);

	int rc;
	if (output_file || optstr("action")) { // cmd-line mode
		if (!inputFiles.size()) help();
		rc = cmdline(optstr("action"), output_file, optstr("d"), inputFiles[0], camera, renderer, original_path, argv[0]);
	} else if (QtUseGUI()) {
		rc = gui(inputFiles, original_path, argc, argv);
	} else {
		PRINT("Requested GUI mode but can't open display!\n");
		help();
	}

	Builtins::instance(true);

	return rc;
}
