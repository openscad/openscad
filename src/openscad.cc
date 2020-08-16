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
#include "RenderStatistic.h"

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

#include "Camera.h"
#include <chrono>
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
using std::unique_ptr;
using boost::lexical_cast;
using boost::bad_lexical_cast;
using boost::is_any_of;

std::string commandline_commands;
static bool arg_info = false;
static std::string arg_colorscheme;

class Echostream
{
public:
	Echostream(std::ostream &stream) : stream(stream)
	{
		set_output_handler(&Echostream::output, this);
	}
	Echostream(const char *filename) : fstream(filename), stream(fstream)
	{
		set_output_handler(&Echostream::output, this);
	}
	static void output(const std::string &msg, void *userdata)
	{
		auto thisp = static_cast<Echostream *>(userdata);
		thisp->stream << msg << "\n";
	}

private:
	std::ofstream fstream;
	std::ostream &stream;
};

static void help(const char *arg0, const po::options_description &desc, bool failure = false)
{
	const fs::path progpath(arg0);
	PRINTB("Usage: %s [options] file.scad\n%s", progpath.filename().string() % STR(desc));
	exit(failure ? 1 : 0);
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
static void version()
{
	PRINTB("OpenSCAD version %s", TOSTRING(OPENSCAD_VERSION));
	exit(0);
}

static int info()
{
	std::cout << LibraryInfo::info() << "\n\n";

	try {
		OffscreenView glview(512,512);
		std::cout << glview.getRendererInfo() << "\n";
	} catch (int error) {
		PRINTB("Can't create OpenGL OffscreenView. Code: %i. Exiting.\n", error);
		return 1;
	}

	return 0;
}

template <typename F>
static bool with_output(const std::string &filename, F f, std::ios::openmode mode = std::ios::out)
{
	if (filename == "-") {
		f(std::cout);
		return true;
	}
	std::ofstream fstream(filename, mode);
	if (!fstream.is_open()) {
		PRINTB("Can't open file \"%s\" for export", filename.c_str());
		return false;
	}
	else {
		f(fstream);
		return true;
	}
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

Camera get_camera(const po::variables_map &vm)
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

int cmdline(const char *deps_output_file, const std::string &filename, const std::string &output_file, const fs::path &original_path, const std::string &parameterFile, const std::string &setName, const ViewOptions& viewOptions, Camera camera, const std::string &export_format)
{
	Tree tree;
	boost::filesystem::path doc(filename);
	tree.setDocumentPath(doc.remove_filename().string());
#ifdef ENABLE_CGAL
	GeometryEvaluator geomevaluator(tree);
#endif

	ExportFileFormatOptions exportFileFormatOptions;
	FileFormat curFormat;
	std::string formatName;
	std::string output_file_str = output_file;
	const char *new_output_file = nullptr;

	// Determine output file format and assign it to formatName
	if(!export_format.empty()) {
		formatName = export_format;
	} else {
		// else extract format from file extension
		auto suffix = fs::path(output_file_str).extension().generic_string();
		if (suffix.length() > 1) {
			// Remove the period
			suffix = suffix.substr(1);
		}
		boost::algorithm::to_lower(suffix);
		if(exportFileFormatOptions.exportFileFormats.find(suffix) != exportFileFormatOptions.exportFileFormats.end()) {
			formatName = suffix;
		} else {
			PRINTB("\nUnknown suffix '%s' for output file %s", suffix % output_file_str);
			PRINT("Either add a valid suffix or specify one using --export-format\n");
			return 1;
		}
	}

	curFormat = exportFileFormatOptions.exportFileFormats.at(formatName);
	std::string filename_str = fs::path(output_file_str).generic_string();
	new_output_file = filename_str.c_str();

	// Do some minimal checking of output directory before rendering (issue #432)
	auto output_path = fs::path(output_file_str).parent_path();
	if (output_path.empty()) {
		// If output_file_str has no directory prefix, set output directory to current directory.
		output_path = fs::current_path();
	}
	if (!fs::is_directory(output_path)) {
		PRINTB(
			"\n'%s' is not a directory for output file %s - Skipping\n",
			output_path.generic_string() % output_file_str
		);
		return 1;
	}

	set_render_color_scheme(arg_colorscheme, true);

	// Top context - this context only holds builtins
	ContextHandle<BuiltinContext> top_ctx{Context::create<BuiltinContext>()};
	const bool preview = canPreview(curFormat) ? (viewOptions.renderer == RenderType::OPENCSG || viewOptions.renderer == RenderType::THROWNTOGETHER) : false;
	top_ctx->set_variable("$preview", ValuePtr(preview));
#ifdef DEBUG
	PRINTDB("BuiltinContext:\n%s", top_ctx->dump(nullptr, nullptr));
#endif
	shared_ptr<Echostream> echostream;
	if (curFormat == FileFormat::ECHO) {
		if (filename_str == "-") {
			echostream.reset(new Echostream(std::cout));
		}
		else {
			echostream.reset(new Echostream(new_output_file));
		}
	}

	FileModule *root_module;
	ModuleInstantiation root_inst("group");
	const AbstractNode *root_node;
	AbstractNode *absolute_root_node;
	shared_ptr<const Geometry> root_geom;
	unique_ptr<OffscreenView> glview;

	handle_dep(filename);

	std::string text;
	if (filename == "-") {
		text =
				std::string((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
	}
	else {
		std::ifstream ifs(filename.c_str());
		if (!ifs.is_open()) {
			PRINTB("Can't open input file '%s'!\n", filename.c_str());
			return 1;
		}
		text = std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	}

	text += "\n\x03\n" + commandline_commands;

	std::string parser_filename = filename == "-" ? "<stdin>" : filename;
	if (!parse(root_module, text, parser_filename, parser_filename, false)) {
		delete root_module; // parse failed
		root_module = nullptr;
	}
	if (!root_module) {
		PRINTB("Can't parse file '%s'!\n", parser_filename.c_str());
		return 1;
	}

	// add parameter to AST
	CommentParser::collectParameters(text.c_str(), root_module);
	if (!parameterFile.empty() && !setName.empty()) {
		ParameterSet param;
		param.readParameterSet(parameterFile);
		param.applyParameterSet(root_module, setName);
	}
    
	root_module->handleDependencies();

	auto fpath = fs::absolute(fs::path(filename));
	auto fparent = fpath.parent_path();
	fs::current_path(fparent);
	top_ctx->setDocumentPath(fparent.string());

	AbstractNode::resetIndexCounter();
	absolute_root_node = root_module->instantiate(top_ctx.ctx, &root_inst, nullptr);

	// Do we have an explicit root node (! modifier)?
	const Location *nextLocation = nullptr;
	if (!(root_node = find_root_tag(absolute_root_node, &nextLocation))) {
		root_node = absolute_root_node;
	}
	tree.setRoot(root_node);
	if (nextLocation) {
		PRINTB("WARNING: More than one Root Modifier (!) %s", nextLocation->toRelativeString(top_ctx->documentPath()));
	}
	fs::current_path(original_path);

	if (deps_output_file) {
		std::string deps_out(deps_output_file);
		std::string geom_out(output_file);
		int result = write_deps(deps_out, geom_out);
		if (!result) {
			PRINT("error writing deps");
			return 1;
		}
	}

	if (curFormat == FileFormat::CSG) {
		fs::current_path(fparent); // Force exported filenames to be relative to document path
		with_output(filename_str, [&tree, root_node](std::ostream &stream) {
			stream << tree.getString(*root_node, "\t") << "\n";
		});
		fs::current_path(original_path);
	}
	else if (curFormat == FileFormat::AST) {
		fs::current_path(fparent); // Force exported filenames to be relative to document path
		with_output(filename_str,
								[root_module](std::ostream &stream) { stream << root_module->dump(""); });
		fs::current_path(original_path);
	}
	else if (curFormat == FileFormat::TERM) {
		CSGTreeEvaluator csgRenderer(tree);
		auto root_raw_term = csgRenderer.buildCSGTree(*root_node);

		with_output(filename_str, [root_raw_term](std::ostream &stream) {
			if (!root_raw_term)
				stream << "No top-level CSG object\n";
			else {
				stream << root_raw_term->dump() << "\n";
			}
		});
	}
	else if (curFormat == FileFormat::ECHO) {
		// echo -> don't need to evaluate any geometry
	}
	else {
#ifdef ENABLE_CGAL
		// start measuring render time
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		if ((curFormat == FileFormat::PNG) && (viewOptions.renderer == RenderType::OPENCSG || viewOptions.renderer == RenderType::THROWNTOGETHER)) {
			// OpenCSG or throwntogether png -> just render a preview
			glview = prepare_preview(tree, viewOptions, camera);
		} else {
			// Force creation of CGAL objects (for testing)
			root_geom = geomevaluator.evaluateGeometry(*tree.root(), true);
			if (root_geom) {
				if (viewOptions.renderer == RenderType::CGAL && root_geom->getDimension() == 3) {
					if (auto geomlist = dynamic_pointer_cast<const GeometryList>(root_geom)) {
						auto flatlist = geomlist->flatten();
						for (auto &child : flatlist) {
							if (child.second->getDimension() == 3 && !dynamic_pointer_cast<const CGAL_Nef_polyhedron>(child.second)) {
								child.second.reset(CGALUtils::createNefPolyhedronFromGeometry(*child.second));
							}
						}
						root_geom.reset(new GeometryList(flatlist));
					} else if (!dynamic_pointer_cast<const CGAL_Nef_polyhedron>(root_geom)) {
						root_geom.reset(CGALUtils::createNefPolyhedronFromGeometry(*root_geom));
					}
					PRINT("Converted to Nef polyhedron");
				}
			} else {
				root_geom.reset(new CGAL_Nef_polyhedron());
			}
		}

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		RenderStatistic::printCacheStatistic();
		RenderStatistic::printRenderingTime( std::chrono::duration_cast<std::chrono::milliseconds>(end-begin) );
		if (root_geom && !root_geom->isEmpty()) {
			RenderStatistic().print(*root_geom);
		}

        if( curFormat == FileFormat::ASCIISTL ||
            curFormat == FileFormat::STL ||
            curFormat == FileFormat::OFF ||
            curFormat == FileFormat::AMF ||
            curFormat == FileFormat::_3MF ||
            curFormat == FileFormat::NEFDBG ||
            curFormat == FileFormat::NEF3 )
        {
            if(!checkAndExport(root_geom, 3, curFormat, new_output_file)) {
                return 1;
            }
		}

		if(curFormat == FileFormat::DXF || curFormat == FileFormat::SVG) {
			if (!checkAndExport(root_geom, 2, curFormat, new_output_file)) {
				return 1;
			}
		}

		if (curFormat == FileFormat::PNG) {
			bool success = true;
			bool wrote = with_output(
					filename_str,
					[&success, root_geom, &viewOptions, &camera, &glview](std::ostream &stream) {
						if (viewOptions.renderer == RenderType::CGAL ||
								viewOptions.renderer == RenderType::GEOMETRY) {
							success = export_png(root_geom, viewOptions, camera, stream);
						}
						else {
							success = export_png(*glview, stream);
						}
					},
					std::ios::out | std::ios::binary);
			return (success && wrote) ? 0 : 1;
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
#include <QStringList>
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
  QFileInfo fileInfo(qsDir, qsFile);
  return fileInfo.absoluteFilePath();
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
	OpenSCADApp app(argc, argv);
	// remove ugly frames in the QStatusBar when using additional widgets
	app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");

	// set up groups for QSettings
	QCoreApplication::setOrganizationName("OpenSCAD");
	QCoreApplication::setOrganizationDomain("openscad.org");
	QCoreApplication::setApplicationName("OpenSCAD");
	QCoreApplication::setApplicationVersion(TOSTRING(OPENSCAD_VERSION));
	QGuiApplication::setApplicationDisplayName("OpenSCAD");
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
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

	QStringList inputFilesList;
	for(const auto &infile: inputFiles) {
		inputFilesList.append(assemblePath(original_path, infile));
	}
	new MainWindow(inputFilesList);
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(releaseQSettingsCached()));
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

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
	if (Feature::ExperimentalInputDriverDBus.is_enabled()) {
		if(s->get(Settings::Settings::inputEnableDriverDBUS).toBool()){
			auto dBusDriver =new DBusInputDriver();
			InputDriverManager::instance()->registerDriver(dBusDriver);
		}
	}
#endif

	InputDriverManager::instance()->init();
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

bool flagConvert(std::string str){
	if(str =="1" || boost::iequals(str, "on") || boost::iequals(str, "true")) {
		return true;
	}
	if(str =="0" || boost::iequals(str, "off") || boost::iequals(str, "false")) {
		return false;
	}
	throw std::runtime_error("");
	return false;
}

// openSCAD
int main(int argc, char **argv)
{
	int rc = 0;
	StackCheck::inst();

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

	vector<string> output_files;
	const char *deps_output_file = nullptr;
	std::string export_format;

	ViewOptions viewOptions{};
	po::options_description desc("Allowed options");
	desc.add_options()
		("export-format", po::value<string>(), "overrides format of exported scad file when using option '-o', arg can be any of its supported file extensions.  For ascii stl export, specify 'asciistl', and for binary stl export, specify 'binstl'.  Ascii export is the current stl default, but binary stl is planned as the future default so asciistl should be explicitly specified in scripts when needed.\n")
		("o,o", po::value<vector<string>>(), "output specified file instead of running the GUI, the file extension specifies the type: stl, off, amf, 3mf, csg, dxf, svg, png, echo, ast, term, nef3, nefdbg. (May be used multiple time for different exports)\n")
		("D,D", po::value<vector<string>>(), "var=val -pre-define variables")
		("p,p", po::value<string>(), "customizer parameter file")
		("P,P", po::value<string>(), "customizer parameter set")
#ifdef ENABLE_EXPERIMENTAL
		("enable", po::value<vector<string>>(), ("enable experimental features: " +
		                                          join(boost::make_iterator_range(Feature::begin(), Feature::end()), " | ",
		                                               [](const Feature *feature) {
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
		                                      join(ColorMap::inst()->colorSchemeNames(), " | ",
		                                           [](const std::string& colorScheme) {
		                                               return (colorScheme == ColorMap::inst()->defaultColorSchemeName() ? "*" : "") + colorScheme;
		                                           }) +
		                                      "\n").c_str())
		("d,d", po::value<string>(), "deps_file -generate a dependency file for make")
		("m,m", po::value<string>(), "make_cmd -runs make_cmd file if file is missing")
		("quiet,q", "quiet mode (don't print anything *except* errors)")
		("hardwarnings", "Stop on the first warning")
		("check-parameters", po::value<string>(), "=true/false, configure the parameter check for user modules and functions")
		("check-parameter-ranges", po::value<string>(), "=true/false, configure the parameter range check for builtin modules")
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

	if (vm.count("hardwarnings")) {
		OpenSCAD::hardwarnings = true;
	}
	
	std::map<std::string, bool*> flags;
	flags.insert(std::make_pair("check-parameters",&OpenSCAD::parameterCheck));
	flags.insert(std::make_pair("check-parameter-ranges",&OpenSCAD::rangeCheck));
	for(auto flag : flags) {
		std::string name = flag.first;
		if(vm.count(name)){
			std::string opt = vm[name].as<string>();
			try {
				(*(flag.second) = flagConvert(opt));
			} catch ( const std::runtime_error &e ) {
				PRINTB("Could not parse '--%s %s' as flag", name % opt);
			}
		}
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
		output_files = vm["o"].as<vector<string>>();
	}
	if (vm.count("s")) {
		printDeprecation("The -s option is deprecated. Use -o instead.\n");
		output_files.push_back(vm["s"].as<string>());
	}
	if (vm.count("x")) {
		printDeprecation("The -x option is deprecated. Use -o instead.\n");
		output_files.push_back(vm["x"].as<string>());
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
	if (vm.count("enable")) {
		for(const auto &feature : vm["enable"].as<vector<string>>()) {
			Feature::enable_feature(feature);
		}
	}

	string parameterFile;
	if (vm.count("p")) {
		if (!parameterFile.empty()) {
			help(argv[0], desc, true);
		}
		parameterFile = vm["p"].as<string>().c_str();
	}

	string parameterSet;
	if (vm.count("P")) {
		if (!parameterSet.empty()) {
			help(argv[0], desc, true);
		}
		parameterSet = vm["P"].as<string>().c_str();
	}
	
	vector<string> inputFiles;
	if (vm.count("input-file"))	{
		inputFiles = vm["input-file"].as<vector<string>>();
	}

	if (vm.count("colorscheme")) {
		arg_colorscheme = vm["colorscheme"].as<string>();
	}

	ExportFileFormatOptions exportFileFormatOptions;
	if(vm.count("export-format")) {
		auto tmp_format = vm["export-format"].as<string>();
		if(exportFileFormatOptions.exportFileFormats.find(tmp_format) != exportFileFormatOptions.exportFileFormats.end()) {
			export_format = tmp_format;
		}
		else {
			PRINTB("\nUnknown --export-format option '%s'.  Use -h to list available options.\n", tmp_format.c_str());
			return 1;
		}
	}

	Camera camera = get_camera(vm);

	auto cmdlinemode = false;
	if (!output_files.empty()) { // cmd-line mode
		cmdlinemode = true;
		if (!inputFiles.size()) help(argv[0], desc, true);
	}

	if (arg_info || cmdlinemode) {
		if (inputFiles.size() > 1) help(argv[0], desc, true);
		try {
			parser_init();
			localization_init();
			if (arg_info) {
				rc = info();
			}
			else {
				for(auto output_file : output_files) {
					rc |= cmdline(deps_output_file, inputFiles[0], output_file, original_path, parameterFile, parameterSet, viewOptions, camera, export_format);
				}
			}
		} catch (const HardWarningException &) {
			rc = 1;
		}
	}
	else if (QtUseGUI()) {
		if(vm.count("export-format")) {
			PRINT("Ignoring --export-format option");
		}
		rc = gui(inputFiles, original_path, argc, argv);
	}
	else {
		PRINT("Requested GUI mode but can't open display!\n");
		return 1;
	}

	Builtins::instance(true);

	return rc;
}
