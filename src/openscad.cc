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

#include <string>
#include <vector>
#include <fstream>

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
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "boosty.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace Render { enum type { CGAL, OPENCSG, THROWNTOGETHER }; };
std::string commandline_commands;
std::string currentdir;
using std::string;
using std::vector;
using boost::lexical_cast;
using boost::is_any_of;

class Echostream : public std::ofstream
{
public:
	Echostream( const char * filename ) : std::ofstream( filename ) {
		set_output_handler( &Echostream::output, this );
	}
	static void output( const std::string &msg, void *userdata ) {
		Echostream *thisp = static_cast<Echostream*>(userdata);
		*thisp << msg << "\n";
	}
	~Echostream() {
		this->close();
	}
};

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

int cmdline(const char *deps_output_file, const std::string &filename, Camera &camera, const char *output_file, const fs::path &original_path, Render::type renderer, int argc, char ** argv )
{
#ifdef OPENSCAD_QTGUI
	QCoreApplication app(argc, argv);
	const std::string application_path = QApplication::instance()->applicationDirPath().toLocal8Bit().constData();
#else
	const std::string application_path = boosty::stringy(boosty::absolute(boost::filesystem::path(argv[0]).parent_path()));
#endif
	parser_init(application_path);
	Tree tree;
#ifdef ENABLE_CGAL
	CGALEvaluator cgalevaluator(tree);
	PolySetCGALEvaluator psevaluator(cgalevaluator);
#endif
	const char *stl_output_file = NULL;
	const char *off_output_file = NULL;
	const char *dxf_output_file = NULL;
	const char *csg_output_file = NULL;
	const char *png_output_file = NULL;
	const char *ast_output_file = NULL;
	const char *term_output_file = NULL;
	const char *echo_output_file = NULL;

	std::string suffix = boosty::extension_str( output_file );
	boost::algorithm::to_lower( suffix );

	if (suffix == ".stl") stl_output_file = output_file;
	else if (suffix == ".off") off_output_file = output_file;
	else if (suffix == ".dxf") dxf_output_file = output_file;
	else if (suffix == ".csg") csg_output_file = output_file;
	else if (suffix == ".png") png_output_file = output_file;
	else if (suffix == ".ast") ast_output_file = output_file;
	else if (suffix == ".term") term_output_file = output_file;
	else if (suffix == ".echo") echo_output_file = output_file;
	else {
		PRINTB("Unknown suffix for output file %s\n", output_file);
		return 1;
	}

	// Top context - this context only holds builtins
	ModuleContext top_ctx;
	top_ctx.registerBuiltin();
#if 0 && DEBUG
	top_ctx.dump(NULL, NULL);
#endif
	shared_ptr<Echostream> echostream;
	if (echo_output_file)
		echostream.reset( new Echostream( echo_output_file ) );

	FileModule *root_module;
	ModuleInstantiation root_inst("group");
	AbstractNode *root_node;
	AbstractNode *absolute_root_node;
	CGAL_Nef_polyhedron root_N;

	handle_dep(filename.c_str());

	std::ifstream ifs(filename.c_str());
	if (!ifs.is_open()) {
		PRINTB("Can't open input file '%s'!\n", filename.c_str());
		return 1;
	}
	std::string text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	text += "\n" + commandline_commands;
	fs::path abspath = boosty::absolute(filename);
	std::string parentpath = boosty::stringy(abspath.parent_path());
	root_module = parse(text.c_str(), parentpath.c_str(), false);
	if (!root_module) {
		PRINTB("Can't parse file '%s'!\n", filename.c_str());
		return 1;
	}
	root_module->handleDependencies();

	fs::path fpath = boosty::absolute(fs::path(filename));
	fs::path fparent = fpath.parent_path();
	fs::current_path(fparent);
	top_ctx.setDocumentPath(fparent.string());

	AbstractNode::resetIndexCounter();
	absolute_root_node = root_module->instantiate(&top_ctx, &root_inst, NULL);

	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node)))
		root_node = absolute_root_node;

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
	else if (ast_output_file) {
		fs::current_path(original_path);
		std::ofstream fstream(ast_output_file);
		if (!fstream.is_open()) {
			PRINTB("Can't open file \"%s\" for export", ast_output_file);
		}
		else {
			fs::current_path(fparent); // Force exported filenames to be relative to document path
			fstream << root_module->dump("", "") << "\n";
			fstream.close();
		}
	}
	else if (term_output_file) {
		std::vector<shared_ptr<CSGTerm> > highlight_terms;
		std::vector<shared_ptr<CSGTerm> > background_terms;

		CSGTermEvaluator csgRenderer(tree, &psevaluator);
		shared_ptr<CSGTerm> root_raw_term = csgRenderer.evaluateCSGTerm(*root_node, highlight_terms, background_terms);

		fs::current_path(original_path);
		std::ofstream fstream(term_output_file);
		if (!fstream.is_open()) {
			PRINTB("Can't open file \"%s\" for export", term_output_file);
		}
		else {
			if (!root_raw_term)
				fstream << "No top-level CSG object\n";
			else {
				fstream << root_raw_term->dump() << "\n";
			}
			fstream.close();
		}
	}
	else {
#ifdef ENABLE_CGAL
		if ((echo_output_file || png_output_file) && !(renderer==Render::CGAL)) {
			// echo or OpenCSG png -> don't necessarily need CGALMesh evaluation
		} else {
			root_N = cgalevaluator.evaluateCGALMesh(*tree.root());
		}

		fs::current_path(original_path);

		if (deps_output_file) {
			std::string deps_out( deps_output_file );
			std::string geom_out;
			if ( stl_output_file ) geom_out = std::string(stl_output_file);
			else if ( off_output_file ) geom_out = std::string(off_output_file);
			else if ( dxf_output_file ) geom_out = std::string(dxf_output_file);
			else if ( png_output_file ) geom_out = std::string(png_output_file);
			else {
				PRINTB("Output file:%s\n",output_file);
				PRINT("Sorry, don't know how to write deps for that file type. Exiting\n");
				return 1;
			}
			int result = write_deps( deps_out, geom_out );
			if ( !result ) {
				PRINT("error writing deps");
				return 1;
			}
		}

		if (stl_output_file) {
			if (root_N.dim != 3) {
				PRINT("Current top level object is not a 3D object.\n");
				return 1;
			}
			if (!root_N.p3->is_simple()) {
				PRINT("Object isn't a valid 2-manifold! Modify your design.\n");
				return 1;
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
				PRINT("Current top level object is not a 3D object.\n");
				return 1;
			}
			if (!root_N.p3->is_simple()) {
				PRINT("Object isn't a valid 2-manifold! Modify your design.\n");
				return 1;
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
				PRINT("Current top level object is not a 2D object.\n");
				return 1;
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

		if (png_output_file) {
			std::ofstream fstream(png_output_file,std::ios::out|std::ios::binary);
			if (!fstream.is_open()) {
				PRINTB("Can't open file \"%s\" for export", png_output_file);
			}
			else {
				if (renderer==Render::CGAL) {
					export_png_with_cgal(&root_N, camera, fstream);
				} else if (renderer==Render::THROWNTOGETHER) {
					export_png_with_throwntogether(tree, camera, fstream);
				} else {
					export_png_with_opencsg(tree, camera, fstream);
				}
				fstream.close();
			}
		}
#else
		PRINT("OpenSCAD has been compiled without CGAL support!\n");
		return 1;
#endif
	}
	delete root_node;
	return 0;
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
	int rc = 0;
#ifdef Q_OS_MAC
	set_output_handler(CocoaUtils::nslog, NULL);
#endif
#ifdef ENABLE_CGAL
	// Causes CGAL errors to abort directly instead of throwing exceptions
	// (which we don't catch). This gives us stack traces without rerunning in gdb.
	CGAL::set_error_behaviour(CGAL::ABORT);
#endif
	Builtins::instance()->initialize();

	fs::path original_path = fs::current_path();

	const char *output_file = NULL;
	const char *deps_output_file = NULL;

	po::options_description opt_actions("Actions (pick one, or none to start the gui)");
	opt_actions.add_options()
	  ("o,o", po::value<string>(), "output file; file extensions determines action:\n"
	                               "  .stl .off .dxf .csg - export geometry\n"
	                               "  .png - render image\n"
	                               "  .ast - export abstract syntax tree"
	                               // TODO ".term"
                                       // TODO ".echo"
	   )
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

	if (vm.count("o")) {
		// FIXME: Allow for multiple output files?
		if (output_file) help();
		output_file = vm["o"].as<string>().c_str();
	}
	if (vm.count("s")) {
		PRINT("DEPRECATED: The -s option is deprecated. Use -o instead.\n");
		if (output_file) help();
		output_file = vm["s"].as<string>().c_str();
	}
	if (vm.count("x")) { 
		PRINT("DEPRECATED: The -x option is deprecated. Use -o instead.\n");
		if (output_file) help();
		output_file = vm["x"].as<string>().c_str();
	}
	if (vm.count("d")) {
		if (deps_output_file)
			help();
		deps_output_file = vm["d"].as<string>().c_str();
	}
	if (vm.count("m")) {
		if (make_command)
			help();
		make_command = vm["m"].as<string>().c_str();
	}

	if (vm.count("D")) {
		BOOST_FOREACH(const string &cmd, vm["D"].as<vector<string> >()) {
			commandline_commands += cmd;
			commandline_commands += ";\n";
		}
	}
	if (vm.count("enable")) {
		BOOST_FOREACH(const string &feature, vm["enable"].as<vector<string> >()) {
			Feature::enable_feature(feature);
		}
	}
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

	bool cmdlinemode = false;
	if (output_file) { // cmd-line mode
		cmdlinemode = true;
		if (!inputFiles.size()) help();
	}

	if (cmdlinemode) {
		rc = cmdline(deps_output_file, inputFiles[0], camera, output_file, original_path, renderer, argc, argv);
	}
	else if (QtUseGUI()) {
		rc = gui(inputFiles, original_path, argc, argv);
	}
	else {
		PRINT("Requested GUI mode but can't open display!\n");
		help();
	}

	Builtins::instance(true);

	return rc;
}

