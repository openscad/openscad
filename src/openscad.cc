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
#include "comment.h"
#include "node.h"
#include "module.h"
#include "ModuleInstantiation.h"
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
#include "LibraryInfo.h"
#include "nodedumper.h"
#include "stackcheck.h"
#include "CocoaUtils.h"
#include "FontCache.h"
#include "OffscreenView.h"
#include "GeometryEvaluator.h"

#include"parameter/parameterset.h"
#include <string>
#include <vector>
#include <fstream>

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgalutils.h"
#endif

#include "csgnode.h"
#include "CSGTreeEvaluator.h"

#include <sstream>

#include "Camera.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#ifdef __APPLE__
#include "AppleEvents.h"
  #ifdef OPENSCAD_UPDATER
    #include "SparkleAutoUpdater.h"
  #endif
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;
enum class RenderType { GEOMETRY, CGAL, OPENCSG, THROWNTOGETHER };
using std::string;
using std::vector;
using boost::lexical_cast;
using boost::bad_lexical_cast;
using boost::is_any_of;

std::string commandline_commands;
std::string currentdir;
static bool arg_info = false;
static std::string arg_colorscheme;

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

std::string openscad_shortversionnumber = QUOTED(OPENSCAD_SHORTVERSION);
std::string openscad_versionnumber = QUOTED(OPENSCAD_VERSION);

std::string openscad_displayversionnumber = 
#ifdef OPENSCAD_COMMIT
  QUOTED(OPENSCAD_VERSION)
  " (git " QUOTED(OPENSCAD_COMMIT) ")";
#else
  QUOTED(OPENSCAD_SHORTVERSION);
#endif

std::string openscad_detailedversionnumber =
#ifdef OPENSCAD_COMMIT
  openscad_displayversionnumber;
#else
  openscad_versionnumber;
#endif

class Echostream : public std::ofstream
{
public:
	Echostream(const char * filename) : std::ofstream(filename) {
		set_output_handler( &Echostream::output, this );
	}
	static void output(const std::string &msg, void *userdata) {
		auto thisp = static_cast<Echostream*>(userdata);
		*thisp << msg << "\n";
	}
	~Echostream() {
		this->close();
	}
};

static void help(const char *progname, bool failure = false)
{
  int tablen = strlen(progname)+8;
  char tabstr[tablen+1];
  for (int i=0;i<tablen;i++) tabstr[i] = ' ';
  tabstr[tablen] = '\0';

	PRINTB("Usage: %1% [ -o output_file [ -d deps_file ] ]\\\n"
         "%2%[ -m make_command ] [ -D var=val [..] ] \\\n"
	 "%2%[ --help ] print this help message and exit \\\n"
         "%2%[ --version ] [ --info ] \\\n"
         "%2%[ --camera=translatex,y,z,rotx,y,z,dist | \\\n"
         "%2%  --camera=eyex,y,z,centerx,y,z ] \\\n"
         "%2%[ --autocenter ] \\\n"
         "%2%[ --viewall ] \\\n"
         "%2%[ --imgsize=width,height ] [ --projection=(o)rtho|(p)ersp] \\\n"
         "%2%[ --render | --preview[=throwntogether] ] \\\n"
         "%2%[ --colorscheme=[Cornfield|Sunset|Metallic|Starnight|BeforeDawn|Nature|DeepOcean] ] \\\n"
         "%2%[ --csglimit=num ] \\\n"
         "%2%[ --view=[axes,showEdges,scaleMarkers] ] \\\n"
#ifdef ENABLE_EXPERIMENTAL
         " [ --enable=<feature> ] \\\n"
         "%2%[ -p <Parameter Filename>] [-P <Parameter Set>] "
#endif
         "\\\n"
#ifdef DEBUG
				 "%2%[ --debug=module ] \\\n"
#endif
         "%2%filename\n",
 				 progname % (const char *)tabstr);
	exit(failure ? 1 : 0);
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
static void version()
{
	PRINTB("OpenSCAD version %s", TOSTRING(OPENSCAD_VERSION));
	exit(0);
}

static void info()
{
	std::cout << LibraryInfo::info() << "\n\n";

	try {
		OffscreenView glview(512,512);
		std::cout << glview.getRendererInfo() << "\n";
	} catch (int error) {
		PRINTB("Can't create OpenGL OffscreenView. Code: %i. Exiting.\n", error);
		exit(1);
	}

	exit(0);
}

/**
 * Initialize gettext. This must be called after the appliation path was
 * determined so we can lookup the resource path for the language translation
 * files.
 */
void localization_init() {
	fs::path po_dir(PlatformUtils::resourcePath("locale"));
	std::string locale_path(po_dir.string());

	if (fs::is_directory(locale_path)) {
		setlocale(LC_ALL, "");
		bindtextdomain("openscad", locale_path.c_str());
		bind_textdomain_codeset("openscad", "UTF-8");
		textdomain("openscad");
	} else {
		PRINT("Could not initialize localization.");
	}
}

Camera get_camera(po::variables_map vm)
{
	Camera camera;

	if (vm.count("camera")) {
		vector<string> strs;
		vector<double> cam_parameters;
		split(strs, vm["camera"].as<string>(), is_any_of(","));
		if (strs.size() == 6 || strs.size() == 7) {
			try {
				for (const auto &s : strs) cam_parameters.push_back(lexical_cast<double>(s));
				camera.setup(cam_parameters);
			}
			catch (bad_lexical_cast &) {
				PRINT("Camera setup requires numbers as parameters");
			}
		} else {
			PRINT("Camera setup requires either 7 numbers for Gimbal Camera");
			PRINT("or 6 numbers for Vector Camera");
			exit(1);
		}
	}

	if (camera.type == Camera::CameraType::GIMBAL) {
		camera.gimbalDefaultTranslate();
	}

	if (vm.count("viewall")) {
		camera.viewall = true;
	}

	if (vm.count("autocenter")) {
		camera.autocenter = true;
	}

	if (vm.count("projection")) {
		auto proj = vm["projection"].as<string>();
		if (proj == "o" || proj == "ortho" || proj == "orthogonal") {
			camera.projection = Camera::ProjectionType::ORTHOGONAL;
		}
		else if (proj=="p" || proj=="perspective") {
			camera.projection = Camera::ProjectionType::PERSPECTIVE;
		}
		else {
			PRINT("projection needs to be 'o' or 'p' for ortho or perspective\n");
			exit(1);
		}
	}

	auto w = RenderSettings::inst()->img_width;
	auto h = RenderSettings::inst()->img_height;
	if (vm.count("imgsize")) {
		vector<string> strs;
		split(strs, vm["imgsize"].as<string>(), is_any_of(","));
		if ( strs.size() != 2 ) {
			PRINT("Need 2 numbers for imgsize");
			exit(1);
		} else {
			try {
				w = lexical_cast<int>(strs[0]);
				h = lexical_cast<int>(strs[1]);
			}
			catch (bad_lexical_cast &) {
				PRINT("Need 2 numbers for imgsize");
			}
		}
	}
	camera.pixel_width = w;
	camera.pixel_height = h;

	return camera;
}

#ifndef OPENSCAD_NOGUI
#include "QSettingsCached.h"
#define OPENSCAD_QTGUI 1
#endif
static bool checkAndExport(shared_ptr<const Geometry> root_geom, unsigned nd,
													 FileFormat format, const char *filename)
{
	if (root_geom->getDimension() != nd) {
		PRINTB("Current top level object is not a %dD object.", nd);
		return false;
	}
	if (root_geom->isEmpty()) {
		PRINT("Current top level object is empty.");
		return false;
	}
	exportFileByName(root_geom, format, filename, filename);
	return true;
}

void set_render_color_scheme(const std::string color_scheme, const bool exit_if_not_found)
{
	if (color_scheme.empty()) {
		return;
	}
	
	if (ColorMap::inst()->findColorScheme(color_scheme)) {
		RenderSettings::inst()->colorscheme = color_scheme;
		return;
	}
		
	if (exit_if_not_found) {
		PRINTB("Unknown color scheme '%s'. Valid schemes:", color_scheme);
		for(const auto &name : ColorMap::inst()->colorSchemeNames()) {
			PRINT(name);
		}
		exit(1);
	} else {
		PRINTB("Unknown color scheme '%s', using default '%s'.", arg_colorscheme % ColorMap::inst()->defaultColorSchemeName());
	}
}

#include <QCoreApplication>

int cmdline(const char *deps_output_file, const std::string &filename, Camera &camera, const char *output_file, const fs::path &original_path, RenderType renderer,const std::string &parameterFile,const std::string &setName, ViewOptions& viewOptions, int argc, char ** argv )
{
#ifdef OPENSCAD_QTGUI
	QCoreApplication app(argc, argv);
	const std::string application_path = QCoreApplication::instance()->applicationDirPath().toLocal8Bit().constData();
#else
	const std::string application_path = fs::absolute(boost::filesystem::path(argv[0]).parent_path()).generic_string();
#endif	
	PlatformUtils::registerApplicationPath(application_path);
	parser_init();
	localization_init();

	Tree tree;
#ifdef ENABLE_CGAL
	GeometryEvaluator geomevaluator(tree);
#endif
	if (arg_info) {
	    info();
	}
	
	const char *stl_output_file = nullptr;
	const char *off_output_file = nullptr;
	const char *amf_output_file = nullptr;
	const char *dxf_output_file = nullptr;
	const char *svg_output_file = nullptr;
	const char *csg_output_file = nullptr;
	const char *png_output_file = nullptr;
	const char *ast_output_file = nullptr;
	const char *term_output_file = nullptr;
	const char *echo_output_file = nullptr;
	const char *nefdbg_output_file = nullptr;
	const char *nef3_output_file = nullptr;

	auto suffix = fs::path(output_file).extension().generic_string();
	boost::algorithm::to_lower(suffix);

	if (suffix == ".stl") stl_output_file = output_file;
	else if (suffix == ".off") off_output_file = output_file;
	else if (suffix == ".amf") amf_output_file = output_file;
	else if (suffix == ".dxf") dxf_output_file = output_file;
	else if (suffix == ".svg") svg_output_file = output_file;
	else if (suffix == ".csg") csg_output_file = output_file;
	else if (suffix == ".png") png_output_file = output_file;
	else if (suffix == ".ast") ast_output_file = output_file;
	else if (suffix == ".term") term_output_file = output_file;
	else if (suffix == ".echo") echo_output_file = output_file;
	else if (suffix == ".nefdbg") nefdbg_output_file = output_file;
	else if (suffix == ".nef3") nef3_output_file = output_file;
	else {
		PRINTB("Unknown suffix for output file %s\n", output_file);
		return 1;
	}

	set_render_color_scheme(arg_colorscheme, true);
	
	// Top context - this context only holds builtins
	ModuleContext top_ctx;
	top_ctx.registerBuiltin();
	bool preview = png_output_file ? (renderer==RenderType::OPENCSG || renderer==RenderType::THROWNTOGETHER) : false;
	top_ctx.set_variable("$preview", ValuePtr(preview));
#ifdef DEBUG
	PRINTDB("Top ModuleContext:\n%s",top_ctx.dump(nullptr, nullptr));
#endif
	shared_ptr<Echostream> echostream;
	if (echo_output_file) {
		echostream.reset(new Echostream(echo_output_file));
	}

	FileModule *root_module;
	ModuleInstantiation root_inst("group");
	AbstractNode *root_node;
	AbstractNode *absolute_root_node;
	shared_ptr<const Geometry> root_geom;

	handle_dep(filename);

	std::ifstream ifs(filename.c_str());
	if (!ifs.is_open()) {
		PRINTB("Can't open input file '%s'!\n", filename.c_str());
		return 1;
	}
	std::string text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	text += "\n" + commandline_commands;
	auto abspath = fs::absolute(filename);
	if (!parse(root_module, text.c_str(), abspath, false)) {
		delete root_module;  // parse failed
		root_module = nullptr;
	}
	if (!root_module) {
		PRINTB("Can't parse file '%s'!\n", filename.c_str());
		return 1;
	}

	if (Feature::ExperimentalCustomizer.is_enabled()) {
		// add parameter to AST
		CommentParser::collectParameters(text.c_str(), root_module);
		if (!parameterFile.empty() && !setName.empty()) {
			ParameterSet param;
			param.readParameterSet(parameterFile);
			param.applyParameterSet(root_module, setName);
		}
	}
    
	root_module->handleDependencies();

	auto fpath = fs::absolute(fs::path(filename));
	auto fparent = fpath.parent_path();
	fs::current_path(fparent);
	top_ctx.setDocumentPath(fparent.string());

	AbstractNode::resetIndexCounter();
	absolute_root_node = root_module->instantiate(&top_ctx, &root_inst, nullptr);

	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node))) {
		root_node = absolute_root_node;
	}
	tree.setRoot(root_node);

	if (deps_output_file) {
		fs::current_path(original_path);
		std::string deps_out(deps_output_file);
		std::string geom_out(output_file);
		int result = write_deps(deps_out, geom_out);
		if (!result) {
			PRINT("error writing deps");
			return 1;
		}
	}

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
			fstream << root_module->dump("", "");
			fstream.close();
		}
	}
	else if (term_output_file) {
		CSGTreeEvaluator csgRenderer(tree);
		auto root_raw_term = csgRenderer.buildCSGTree(*root_node);

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
		if ((echo_output_file || png_output_file) &&
				(renderer == RenderType::OPENCSG || renderer == RenderType::THROWNTOGETHER)) {
			// echo or OpenCSG png -> don't necessarily need geometry evaluation
		} else {
			// Force creation of CGAL objects (for testing)
			root_geom = geomevaluator.evaluateGeometry(*tree.root(), true);
			if (!root_geom) root_geom.reset(new CGAL_Nef_polyhedron());
			if (renderer == RenderType::CGAL && root_geom->getDimension() == 3) {
				auto N = dynamic_cast<const CGAL_Nef_polyhedron*>(root_geom.get());
				if (!N) {
					N = CGALUtils::createNefPolyhedronFromGeometry(*root_geom);
					root_geom.reset(N);
					PRINT("Converted to Nef polyhedron");
				}
			}
		}

		fs::current_path(original_path);

		if (stl_output_file) {
			if (!checkAndExport(root_geom, 3, FileFormat::STL, stl_output_file)) {
				return 1;
			}
		}

		if (off_output_file) {
			if (!checkAndExport(root_geom, 3, FileFormat::OFF, off_output_file)) {
				return 1;
			}
		}

		if (amf_output_file) {
			if (!checkAndExport(root_geom, 3, FileFormat::AMF, amf_output_file)) {
				return 1;
			}
		}

		if (dxf_output_file) {
			if (!checkAndExport(root_geom, 2, FileFormat::DXF, dxf_output_file)) {
				return 1;
			}
		}
		
		if (svg_output_file) {
			if (!checkAndExport(root_geom, 2, FileFormat::SVG, svg_output_file)) {
				return 1;
			}
		}

		if (png_output_file) {
			auto success = true;
			std::ofstream fstream(png_output_file,std::ios::out|std::ios::binary);
			if (!fstream.is_open()) {
				PRINTB("Can't open file \"%s\" for export", png_output_file);
				success = false;
			}
			else {
				if (renderer == RenderType::CGAL || renderer == RenderType::GEOMETRY) {
					success = export_png(root_geom, camera, viewOptions, fstream);
				} else {
					viewOptions.previewer = (renderer == RenderType::THROWNTOGETHER) ? Previewer::THROWNTOGETHER : Previewer::OPENCSG;
					success = export_preview_png(tree, camera, viewOptions, fstream);
				}
				fstream.close();
			}
			return success ? 0 : 1;
		}

		if (nefdbg_output_file) {
			if (!checkAndExport(root_geom, 3, FileFormat::NEFDBG, nefdbg_output_file)) {
				return 1;
			}
		}

		if (nef3_output_file) {
			if (!checkAndExport(root_geom, 3, FileFormat::NEF3, nef3_output_file)) {
				return 1;
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

#ifdef OPENSCAD_QTGUI
#include <QtPlugin>
#if defined(__MINGW64__) || defined(__MINGW32__) || defined(_MSCVER)
#if QT_VERSION < 0x050000
Q_IMPORT_PLUGIN(qtaccessiblewidgets)
#endif // QT_VERSION
#endif // MINGW64/MINGW32/MSCVER
#include "MainWindow.h"
#include "OpenSCADApp.h"
#include "launchingscreen.h"
#include "QSettingsCached.h"
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QMetaType>
#include <QTextCodec>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrentRun>

Q_DECLARE_METATYPE(shared_ptr<const Geometry>);

// Only if "fileName" is not absolute, prepend the "absoluteBase".
static QString assemblePath(const fs::path& absoluteBaseDir,
                            const string& fileName) {
  if (fileName.empty()) return "";
  auto qsDir = QString::fromLocal8Bit(absoluteBaseDir.generic_string().c_str());
  auto qsFile = QString::fromLocal8Bit(fileName.c_str());
  // if qsfile is absolute, dir is ignored. (see documentation of QFileInfo)
  QFileInfo info(qsDir, qsFile);
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

void dialogThreadFunc(FontCacheInitializer *initializer)
{
	 initializer->run();
}

void dialogInitHandler(FontCacheInitializer *initializer, void *)
{
	QFutureWatcher<void> futureWatcher;
	QObject::connect(&futureWatcher, SIGNAL(finished()), scadApp, SLOT(hideFontCacheDialog()));

	auto future = QtConcurrent::run(boost::bind(dialogThreadFunc, initializer));
	futureWatcher.setFuture(future);

	// We don't always get the started() signal, so we start manually
	QMetaObject::invokeMethod(scadApp, "showFontCacheDialog");

	// Block, in case we're in a separate thread, or the dialog was closed by the user
	futureWatcher.waitForFinished();

	// We don't always receive the finished signal. We still need the signal to break 
	// out of the exec() though.
	QMetaObject::invokeMethod(scadApp, "hideFontCacheDialog");
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
	OpenSCADApp app(argc, argv);
	// remove ugly frames in the QStatusBar when using additional widgets
	app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");

	// set up groups for QSettings
	QCoreApplication::setOrganizationName("OpenSCAD");
	QCoreApplication::setOrganizationDomain("openscad.org");
	QCoreApplication::setApplicationName("OpenSCAD");
	QCoreApplication::setApplicationVersion(TOSTRING(OPENSCAD_VERSION));
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	QGuiApplication::setApplicationDisplayName("OpenSCAD");
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#else
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif
#ifdef OPENSCAD_SNAPSHOT
	app.setWindowIcon(QIcon(":/icons/openscad-nightly.png"));
#else
	app.setWindowIcon(QIcon(":/icons/openscad.png"));
#endif
	
	// Other global settings
	qRegisterMetaType<shared_ptr<const Geometry>>();
	
	const auto &app_path = app.applicationDirPath();
	PlatformUtils::registerApplicationPath(app_path.toLocal8Bit().constData());

	FontCache::registerProgressHandler(dialogInitHandler);

	parser_init();

	QSettings settings;
	if (settings.value("advanced/localization", true).toBool()) {
		localization_init();
	}

#ifdef Q_OS_MAC
	installAppleEventHandlers();
#endif

#ifdef Q_OS_WIN
	QSettings reg_setting(QLatin1String("HKEY_CURRENT_USER"), QSettings::NativeFormat);
	auto appPath = QDir::toNativeSeparators(app.applicationFilePath() + QLatin1String(",1"));
	reg_setting.setValue(QLatin1String("Software/Classes/OpenSCAD_File/DefaultIcon/Default"),QVariant(appPath));
#endif

#ifdef OPENSCAD_UPDATER
	AutoUpdater *updater = new SparkleAutoUpdater;
	AutoUpdater::setUpdater(updater);
	if (updater->automaticallyChecksForUpdates()) updater->checkForUpdates();
	updater->init();
#endif

#ifndef USE_QOPENGLWIDGET
	// This workaround appears to only be needed when QGLWidget is used QOpenGLWidget
	// available in Qt 5.4 is much better.
	QGLFormat fmt;
#if 0 /*** disabled by clifford wolf: adds rendering artefacts with OpenCSG ***/
	// turn on anti-aliasing
	fmt.setSampleBuffers(true);
	fmt.setSamples(4);
#endif
	// The default SwapInterval causes very bad interactive behavior as
	// waiting for the buffer swap seems to block mouse events. So the
	// effect is that we can process mouse events at the frequency of
	// the screen retrace interval causing them to queue up.
	// (see https://bugreports.qt-project.org/browse/QTBUG-39370
	fmt.setSwapInterval(0);
	QGLFormat::setDefaultFormat(fmt);
#endif

	set_render_color_scheme(arg_colorscheme, false);
	
	auto noInputFiles = false;
	if (!inputFiles.size()) {
		noInputFiles = true;
		inputFiles.push_back("");
	}

	auto showOnStartup = settings.value("launcher/showOnStartup");
	if (noInputFiles && (showOnStartup.isNull() || showOnStartup.toBool())) {
		auto launcher = new LaunchingScreen();
		auto dialogResult = launcher->exec();
		if (dialogResult == QDialog::Accepted) {
			auto files = launcher->selectedFiles();
			// If nothing is selected in the launching screen, leave
			// the "" dummy in inputFiles to open an empty MainWindow.
			if (!files.empty()) {
				inputFiles.clear();
				for (const auto &f : files) {
					inputFiles.push_back(f.toStdString());
				}
			}
			delete launcher;
		} else {
			return 0;
		}
	}

	auto isMdi = settings.value("advanced/mdi", true).toBool();
	if (isMdi) {
		for(const auto &infile : inputFiles) {
		   new MainWindow(assemblePath(original_path, infile));
	    }
	} else {
	   new MainWindow(assemblePath(original_path, inputFiles[0]));
	}

	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
	int rc = app.exec();
	for (auto &mainw : scadApp->windowManager.getWindows()) delete mainw;
	return rc;
}
#else // OPENSCAD_QTGUI
bool QtUseGUI() { return false; }
int gui(const vector<string> &inputFiles, const fs::path &original_path, int argc, char ** argv)
{
	PRINT("Error: compiled without QT, but trying to run GUI\n");
	return 1;
}
#endif // OPENSCAD_QTGUI

std::pair<string, string> customSyntax(const string& s)
{
#if defined(Q_OS_MACX)
	if (s.find("-psn_") == 0)
		return {"psn", s.substr(5)};
#endif

	return {};
}
/*!
	This makes boost::program_option parse comma-separated values
 */
struct CommaSeparatedVector
{
	std::vector<std::string> values;

	friend std::istream &operator>>(std::istream &in, CommaSeparatedVector &value) {
		std::string token;
		in >> token;
		boost::split(value.values, token, boost::is_any_of(","));
		return in;
	}
};

int main(int argc, char **argv)
{
	int rc = 0;
	StackCheck::inst()->init();
	
#ifdef Q_OS_MAC
	bool isGuiLaunched = getenv("GUI_LAUNCHED") != 0;
	if (isGuiLaunched) set_output_handler(CocoaUtils::nslog, nullptr);
#else
	PlatformUtils::ensureStdIO();
#endif

#ifdef ENABLE_CGAL
	// Causes CGAL errors to abort directly instead of throwing exceptions
	// (which we don't catch). This gives us stack traces without rerunning in gdb.
	CGAL::set_error_behaviour(CGAL::ABORT);
#endif
	Builtins::instance()->initialize();

	auto original_path = fs::current_path();

	const char *output_file = nullptr;
	const char *deps_output_file = nullptr;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "help message")
		("version,v", "print the version")
		("info", "print information about the building process")
		("render", po::value<string>()->implicit_value(""), "if exporting a png image, do a full geometry evaluation")
		("preview", po::value<string>()->implicit_value(""), "if exporting a png image, do an OpenCSG(default) or ThrownTogether preview")
		("view", po::value<CommaSeparatedVector>()->value_name("axes|scaleMarkers|showEdges"), "view options")
		("csglimit", po::value<unsigned int>(), "if exporting a png image, stop rendering at the given number of CSG elements")
		("camera", po::value<string>(), "parameters for camera when exporting png")
		("autocenter", "adjust camera to look at object center")
		("viewall", "adjust camera to fit object")
		("imgsize", po::value<string>(), "=width,height for exporting png")
		("projection", po::value<string>(), "(o)rtho or (p)erspective when exporting png")
		("colorscheme", po::value<string>(), "colorscheme")
		("debug", po::value<string>(), "special debug info")
		("quiet,q", "quiet mode (don't print anything *except* errors)")
		("o,o", po::value<string>(), "out-file")
		("p,p", po::value<string>(), "parameter file")
		("P,P", po::value<string>(), "parameter set")
		("s,s", po::value<string>(), "stl-file")
		("x,x", po::value<string>(), "dxf-file")
		("d,d", po::value<string>(), "deps-file")
		("m,m", po::value<string>(), "makefile")
		("D,D", po::value<vector<string>>(), "var=val")
#ifdef Q_OS_MACX
		("psn", po::value<string>(), "process serial number")
#endif
#ifdef ENABLE_EXPERIMENTAL
		("enable", po::value<vector<string>>(), "enable experimental features")
#endif
		;

	po::options_description hidden("Hidden options");
	hidden.add_options()
		("input-file", po::value< vector<string>>(), "input file");

	po::positional_options_description p;
	p.add("input-file", -1);

	po::options_description all_options;
	all_options.add(desc).add(hidden);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(all_options).allow_unregistered().positional(p).extra_parser(customSyntax).run(), vm);
	}
	catch(const std::exception &e) { // Catches e.g. unknown options
		PRINTB("%s\n", e.what());
		help(argv[0], true);
	}

	OpenSCAD::debug = "";
	if (vm.count("debug")) {
		OpenSCAD::debug = vm["debug"].as<string>();
		PRINTB("Debug on. --debug=%s",OpenSCAD::debug);
	}
	if (vm.count("quiet")) {
		OpenSCAD::quiet = true;
	}
	if (vm.count("help")) help(argv[0]);
	if (vm.count("version")) version();
	if (vm.count("info")) arg_info = true;

	auto renderer = RenderType::OPENCSG;
	if (vm.count("preview")) {
		if (vm["preview"].as<string>() == "throwntogether")
			renderer = RenderType::THROWNTOGETHER;
	}
	else if (vm.count("render")) {
		if (vm["render"].as<string>() == "cgal") renderer = RenderType::CGAL;
		else renderer = RenderType::GEOMETRY;
	}

	ViewOptions viewOptions{};
	if (vm.count("view")) {
		const auto &viewOptionValues = vm["view"].as<CommaSeparatedVector>();
		std::cout << "View options:\n";
		for (const auto &option : viewOptionValues.values) {
			if (option == "axes") viewOptions.showAxes = true;
			else if (option == "scaleMarkers") viewOptions.showScaleMarkers = true;
			else if (option == "showEdges") viewOptions.showEdges = true;
			std::cout << option << "\n";
		}
	}

	if (vm.count("csglimit")) {
		RenderSettings::inst()->openCSGTermLimit = vm["csglimit"].as<unsigned int>();
	}

	if (vm.count("o")) {
		// FIXME: Allow for multiple output files?
		if (output_file) help(argv[0], true);
		output_file = vm["o"].as<string>().c_str();
	}
	if (vm.count("s")) {
		printDeprecation("The -s option is deprecated. Use -o instead.\n");
		if (output_file) help(argv[0], true);
		output_file = vm["s"].as<string>().c_str();
	}
	if (vm.count("x")) { 
		printDeprecation("The -x option is deprecated. Use -o instead.\n");
		if (output_file) help(argv[0], true);
		output_file = vm["x"].as<string>().c_str();
	}
	if (vm.count("d")) {
		if (deps_output_file) help(argv[0], true);
		deps_output_file = vm["d"].as<string>().c_str();
	}
	if (vm.count("m")) {
		if (make_command) help(argv[0], true);
		make_command = vm["m"].as<string>().c_str();
	}

	if (vm.count("D")) {
		for(const auto &cmd : vm["D"].as<vector<string>>()) {
			commandline_commands += cmd;
			commandline_commands += ";\n";
		}
	}
#ifdef ENABLE_EXPERIMENTAL
	if (vm.count("enable")) {
		for(const auto &feature : vm["enable"].as<vector<string>>()) {
			Feature::enable_feature(feature);
		}
	}
#endif

	string parameterFile;
	string parameterSet;
	
	if (Feature::ExperimentalCustomizer.is_enabled()) {
		if (vm.count("p")) {
			if (!parameterFile.empty()) help(argv[0], true);
			
			parameterFile = vm["p"].as<string>().c_str();
		}
		
		if (vm.count("P")) {
			if (!parameterSet.empty()) help(argv[0], true);
			
			parameterSet = vm["P"].as<string>().c_str();
		}
	}
	else {
		if (vm.count("p") || vm.count("P")) {
			if (!parameterSet.empty()) help(argv[0], true);
			PRINT("Customizer feature not activated\n");
			help(argv[0], true);
		}
	}
	
	vector<string> inputFiles;
	if (vm.count("input-file"))	{
		inputFiles = vm["input-file"].as<vector<string>>();
	}

	if (vm.count("colorscheme")) {
		arg_colorscheme = vm["colorscheme"].as<string>();
	}

	currentdir = fs::current_path().generic_string();

	Camera camera = get_camera(vm);

	// Initialize global visitors
	NodeCache nodecache;
	NodeDumper dumper(nodecache);

	auto cmdlinemode = false;
	if (output_file) { // cmd-line mode
		cmdlinemode = true;
		if (!inputFiles.size()) help(argv[0], true);
	}

	if (arg_info || cmdlinemode) {
		if (inputFiles.size() > 1) help(argv[0], true);
		rc = cmdline(deps_output_file, inputFiles[0], camera, output_file, original_path, renderer, parameterFile, parameterSet, viewOptions, argc, argv);
	}
	else if (QtUseGUI()) {
		rc = gui(inputFiles, original_path, argc, argv);
	}
	else {
		PRINT("Requested GUI mode but can't open display!\n");
		help(argv[0], true);
	}

	Builtins::instance(true);

	return rc;
}
