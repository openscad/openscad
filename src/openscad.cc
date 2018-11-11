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
#include "FileModule.h"
#include "ModuleInstantiation.h"
#include "builtincontext.h"
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
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
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

static void help(const char *arg0, const po::options_description &desc, bool failure = false)
{
	std::stringstream ss;
	ss << desc;
	const fs::path progpath(arg0);
	PRINTB("Usage: %s [options] file.scad\n%s", progpath.filename().string() % ss.str());
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
 * Initialize gettext. This must be called after the application path was
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
		boost::split(strs, vm["camera"].as<string>(), is_any_of(","));
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
	else {
		camera.viewall = true;
		camera.autocenter = true;
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
		boost::split(strs, vm["imgsize"].as<string>(), is_any_of(","));
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
		PRINT(boost::join(ColorMap::inst()->colorSchemeNames(), "\n"));
		exit(1);
	} else {
		PRINTB("Unknown color scheme '%s', using default '%s'.", arg_colorscheme % ColorMap::inst()->defaultColorSchemeName());
	}
}

int cmdline(const char *deps_output_file, const std::string &filename, const char *output_file, const fs::path &original_path, const std::string &parameterFile, const std::string &setName, const ViewOptions& viewOptions, Camera camera)
{
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
	const char *_3mf_output_file = nullptr;
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
	else if (Feature::Experimental3mfExport.is_enabled() && suffix == ".3mf") _3mf_output_file = output_file;
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
	BuiltinContext top_ctx;
	const bool preview = png_output_file ? (viewOptions.renderer == RenderType::OPENCSG || viewOptions.renderer == RenderType::THROWNTOGETHER) : false;
	top_ctx.set_variable("$preview", ValuePtr(preview));
#ifdef DEBUG
	PRINTDB("BuiltinContext:\n%s", top_ctx.dump(nullptr, nullptr));
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
	text += "\n\x03\n" + commandline_commands;
	if (!parse(root_module, text.c_str(), filename, false)) {
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
			fstream << tree.getString(*root_node, "\t") << "\n";
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
			fstream << root_module->dump("");
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
		if ((echo_output_file || png_output_file) && (viewOptions.renderer == RenderType::OPENCSG || viewOptions.renderer == RenderType::THROWNTOGETHER)) {
			// echo or OpenCSG png -> don't necessarily need geometry evaluation
		} else {
			// Force creation of CGAL objects (for testing)
			root_geom = geomevaluator.evaluateGeometry(*tree.root(), true);
			if (!root_geom) root_geom.reset(new CGAL_Nef_polyhedron());
			if (viewOptions.renderer == RenderType::CGAL && root_geom->getDimension() == 3) {
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

		if (_3mf_output_file) {
			if (!checkAndExport(root_geom, 3, FileFormat::_3MF, _3mf_output_file))
				return 1;
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
				if (viewOptions.renderer == RenderType::CGAL || viewOptions.renderer == RenderType::GEOMETRY) {
					success = export_png(root_geom, viewOptions, camera, fstream);
				} else {
					success = export_preview_png(tree, viewOptions, camera, fstream);
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
#include "input/InputDriverManager.h"
#ifdef ENABLE_HIDAPI
#include "input/HidApiInputDriver.h"
#endif
#ifdef ENABLE_SPNAV
#include "input/SpaceNavInputDriver.h"
#endif
#ifdef ENABLE_JOYSTICK
#include "input/JoystickInputDriver.h"
#endif
#ifdef ENABLE_DBUS
#include "input/DBusInputDriver.h"
#endif
#ifdef ENABLE_QGAMEPAD
#include "input/QGamepadInputDriver.h"
#endif
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QMetaType>
#include <QTextCodec>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include "settings.h"

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

#ifdef Q_OS_WIN
void registerDefaultIcon(QString applicationFilePath) {
	// Not using cached instance here, so this needs to be in a
	// separate scope to ensure the QSettings instance is released
	// directly after use.
	QSettings reg_setting(QLatin1String("HKEY_CURRENT_USER"), QSettings::NativeFormat);
	auto appPath = QDir::toNativeSeparators(applicationFilePath + QLatin1String(",1"));
	reg_setting.setValue(QLatin1String("Software/Classes/OpenSCAD_File/DefaultIcon/Default"),QVariant(appPath));
}
#else
void registerDefaultIcon(QString) { }
#endif

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

	FontCache::registerProgressHandler(dialogInitHandler);

	parser_init();

	QSettingsCached settings;
	if (settings.value("advanced/localization", true).toBool()) {
		localization_init();
	}

#ifdef Q_OS_MAC
	installAppleEventHandlers();
#endif

        registerDefaultIcon(app.applicationFilePath());

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
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(releaseQSettingsCached()));
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

	if (Feature::ExperimentalInputDriver.is_enabled()) {
		auto *s = Settings::Settings::inst();
#ifdef ENABLE_HIDAPI
		if(s->get(Settings::Settings::inputEnableDriverHIDAPI).toBool()){
			auto hidApi = new HidApiInputDriver();
			InputDriverManager::instance()->registerDriver(hidApi);
		}
#endif
#ifdef ENABLE_SPNAV
		if(s->get(Settings::Settings::inputEnableDriverSPNAV).toBool()){
			auto spaceNavDriver = new SpaceNavInputDriver();
			bool spaceNavDominantAxisOnly = s->get(Settings::Settings::inputEnableDriverHIDAPI).toBool();
			spaceNavDriver->setDominantAxisOnly(spaceNavDominantAxisOnly);
			InputDriverManager::instance()->registerDriver(spaceNavDriver);
        }
#endif
#ifdef ENABLE_JOYSTICK
		if(s->get(Settings::Settings::inputEnableDriverJOYSTICK).toBool()){
			std::string nr = s->get(Settings::Settings::joystickNr).toString();
			auto joyDriver = new JoystickInputDriver();
			joyDriver->setJoystickNr(nr);
			InputDriverManager::instance()->registerDriver(joyDriver);
		}
#endif
#ifdef ENABLE_QGAMEPAD
		if(s->get(Settings::Settings::inputEnableDriverQGAMEPAD).toBool()){
			auto qGamepadDriver = new QGamepadInputDriver();
			InputDriverManager::instance()->registerDriver(qGamepadDriver);
		}
#endif
#ifdef ENABLE_DBUS
	if(s->get(Settings::Settings::inputEnableDriverDBUS).toBool()){
			auto dBusDriver =new DBusInputDriver();
			InputDriverManager::instance()->registerDriver(dBusDriver);
		}
#endif
		InputDriverManager::instance()->init();
	}
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

#if defined(Q_OS_MACX)
std::pair<string, string> customSyntax(const string& s)
{
	if (s.find("-psn_") == 0)
		return {"psn", s.substr(5)};
#else
std::pair<string, string> customSyntax(const string&)
{
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

template <class Seq, typename ToString>
std::string join(const Seq &seq, const std::string &sep, const ToString &toString)
{
    return boost::algorithm::join(boost::adaptors::transform(seq, toString), sep);
}

int main(int argc, char **argv)
{
	int rc = 0;
	StackCheck::inst()->init();

#ifdef OPENSCAD_QTGUI
	{   // Need a dummy app instance to get the application path but it needs to be destroyed before the GUI is launched.
		QCoreApplication app(argc, argv);
		PlatformUtils::registerApplicationPath(app.applicationDirPath().toLocal8Bit().constData());
	}
#else
	PlatformUtils::registerApplicationPath(fs::absolute(boost::filesystem::path(argv[0]).parent_path()).generic_string());
#endif
	
#ifdef Q_OS_MAC
	bool isGuiLaunched = getenv("GUI_LAUNCHED") != nullptr;
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
	
	ViewOptions viewOptions{};
	po::options_description desc("Allowed options");
	desc.add_options()
		("o,o", po::value<string>(), "output specified file instead of running the GUI, the file extension specifies the type: stl, off, amf, csg, dxf, svg, png, echo, ast, term, nef3, nefdbg\n")
		("D,D", po::value<vector<string>>(), "var=val -pre-define variables")
#ifdef ENABLE_EXPERIMENTAL
		("p,p", po::value<string>(), "customizer parameter file")
		("P,P", po::value<string>(), "customizer parameter set")
		("enable", po::value<vector<string>>(), ("enable experimental features: " +
																						 join(boost::make_iterator_range(Feature::begin(), Feature::end()), " | ", [](const Feature *feature) {
																								 return feature->get_name();
																							 }) +
																						 "\n").c_str())
#endif
		("help,h", "print this help message and exit")
		("version,v", "print the version")
		("info", "print information about the build process\n")

		("camera", po::value<string>(), "camera parameters when exporting png: =translate_x,y,z,rot_x,y,z,dist or =eye_x,y,z,center_x,y,z")
		("autocenter", "adjust camera to look at object's center")
		("viewall", "adjust camera to fit object")
		("imgsize", po::value<string>(), "=width,height of exported png")
		("render", po::value<string>()->implicit_value(""), "for full geometry evaluation when exporting png")
		("preview", po::value<string>()->implicit_value(""), "[=throwntogether] -for ThrownTogether preview png")
		("view", po::value<CommaSeparatedVector>(), ("=view options: " + boost::join(viewOptions.names(), " | ")).c_str())
		("projection", po::value<string>(), "=(o)rtho or (p)erspective when exporting png")
		("csglimit", po::value<unsigned int>(), "=n -stop rendering at n CSG elements when exporting png")
		("colorscheme", po::value<string>(), ("=colorscheme: " +
																					join(ColorMap::inst()->colorSchemeNames(), " | ", [](const std::string& colorScheme) {
																							return (ColorMap::inst()->defaultColorSchemeName() ? "*" : "") + colorScheme;
																						}) +
																					"\n").c_str())

		("d,d", po::value<string>(), "deps_file -generate a dependency file for make")
		("m,m", po::value<string>(), "make_cmd -runs make_cmd file if file is missing")
		("quiet,q", "quiet mode (don't print anything *except* errors)")
		("debug", po::value<string>(), "special debug info")
		("s,s", po::value<string>(), "stl_file deprecated, use -o")
		("x,x", po::value<string>(), "dxf_file deprecated, use -o")
		;

	po::options_description hidden("Hidden options");
	hidden.add_options()
#ifdef Q_OS_MACX
		("psn", po::value<string>(), "process serial number")
#endif
		("input-file", po::value< vector<string>>(), "input file");

	po::positional_options_description p;
	p.add("input-file", -1);

	po::options_description all_options;
	all_options.add(desc).add(hidden);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(all_options).positional(p).extra_parser(customSyntax).run(), vm);
	}
	catch(const std::exception &e) { // Catches e.g. unknown options
		PRINTB("%s\n", e.what());
		help(argv[0], desc, true);
	}

	OpenSCAD::debug = "";
	if (vm.count("debug")) {
		OpenSCAD::debug = vm["debug"].as<string>();
		PRINTB("Debug on. --debug=%s",OpenSCAD::debug);
	}
	if (vm.count("quiet")) {
		OpenSCAD::quiet = true;
	}
	if (vm.count("help")) help(argv[0], desc);
	if (vm.count("version")) version();
	if (vm.count("info")) arg_info = true;

	if (vm.count("preview")) {
		if (vm["preview"].as<string>() == "throwntogether")
			viewOptions.renderer = RenderType::THROWNTOGETHER;
	}
	else if (vm.count("render")) {
		if (vm["render"].as<string>() == "cgal") viewOptions.renderer = RenderType::CGAL;
		else viewOptions.renderer = RenderType::GEOMETRY;
	}

	viewOptions.previewer = (viewOptions.renderer == RenderType::THROWNTOGETHER) ? Previewer::THROWNTOGETHER : Previewer::OPENCSG;
	if (vm.count("view")) {
		const auto &viewOptionValues = vm["view"].as<CommaSeparatedVector>();

		for (const auto &option : viewOptionValues.values) {
			try {
				viewOptions[option] = true;
			} catch (const std::out_of_range &e) {
				PRINTB("Unknown --view option '%s' ignored. Use -h to list available options.", option);
			}
		}
	}

	if (vm.count("csglimit")) {
		RenderSettings::inst()->openCSGTermLimit = vm["csglimit"].as<unsigned int>();
	}

	if (vm.count("o")) {
		// FIXME: Allow for multiple output files?
		if (output_file) help(argv[0], desc, true);
		output_file = vm["o"].as<string>().c_str();
	}
	if (vm.count("s")) {
		printDeprecation("The -s option is deprecated. Use -o instead.\n");
		if (output_file) help(argv[0], desc, true);
		output_file = vm["s"].as<string>().c_str();
	}
	if (vm.count("x")) {
		printDeprecation("The -x option is deprecated. Use -o instead.\n");
		if (output_file) help(argv[0], desc, true);
		output_file = vm["x"].as<string>().c_str();
	}
	if (vm.count("d")) {
		if (deps_output_file) help(argv[0], desc, true);
		deps_output_file = vm["d"].as<string>().c_str();
	}
	if (vm.count("m")) {
		if (make_command) help(argv[0], desc, true);
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
			if (!parameterFile.empty()) help(argv[0], desc, true);
			
			parameterFile = vm["p"].as<string>().c_str();
		}
		
		if (vm.count("P")) {
			if (!parameterSet.empty()) help(argv[0], desc, true);
			
			parameterSet = vm["P"].as<string>().c_str();
		}
	}
	else {
		if (vm.count("p") || vm.count("P")) {
			if (!parameterSet.empty()) help(argv[0], desc, true);
			PRINT("Customizer feature not activated\n");
			help(argv[0], desc, true);
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

	auto cmdlinemode = false;
	if (output_file) { // cmd-line mode
		cmdlinemode = true;
		if (!inputFiles.size()) help(argv[0], desc, true);
	}

	if (arg_info || cmdlinemode) {
		if (inputFiles.size() > 1) help(argv[0], desc, true);
		rc = cmdline(deps_output_file, inputFiles[0], output_file, original_path, parameterFile, parameterSet, viewOptions, camera);
	}
	else if (QtUseGUI()) {
		rc = gui(inputFiles, original_path, argc, argv);
	}
	else {
		PRINT("Requested GUI mode but can't open display!\n");
		help(argv[0], desc, true);
	}

	Builtins::instance(true);

	return rc;
}
