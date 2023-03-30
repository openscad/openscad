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
#include "CommentParser.h"
#include "core/node.h"
#include "SourceFile.h"
#include "BuiltinContext.h"
#include "Value.h"
#include "export.h"
#include "Builtins.h"
#include "printutils.h"
#include "handle_dep.h"
#include "Feature.h"
#include "parsersettings.h"
#include "RenderSettings.h"
#include "PlatformUtils.h"
#include "LibraryInfo.h"
#include "StackCheck.h"
#include "FontCache.h"
#include "OffscreenView.h"
#include "GeometryEvaluator.h"
#include "RenderStatistic.h"
#include "ParameterObject.h"
#include "ParameterSet.h"
#include "openscad_mimalloc.h"
#include <string>
#include <vector>
#include <fstream>

#ifdef ENABLE_CGAL

#include "CGAL_Nef_polyhedron.h"
#include "cgalutils.h"
#endif

#include "CSGNode.h"
#include "CSGTreeEvaluator.h"

#include "Camera.h"
#include <chrono>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/bind/bind.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#ifdef __APPLE__
#include "CocoaUtils.h"
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
  Echostream(std::ostream& stream) : stream(stream)
  {
    set_output_handler(&Echostream::output, nullptr, this);
  }
  Echostream(const std::string& filename) : fstream(filename), stream(fstream)
  {
    set_output_handler(&Echostream::output, nullptr, this);
  }
  static void output(const Message& msgObj, void *userdata)
  {
    auto self = static_cast<Echostream *>(userdata);
    self->stream << msgObj.str() << "\n";
  }
  ~Echostream() {
    if (fstream.is_open()) fstream.close();
  }

private:
  std::ofstream fstream;
  std::ostream& stream;
};

static void help(const char *arg0, const po::options_description& desc, bool failure = false)
{
  const fs::path progpath(arg0);
  LOG("Usage: %1$s [options] file.scad\n%2$s", progpath.filename().string(), desc);
  exit(failure ? 1 : 0);
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
static void version()
{
  LOG("OpenSCAD version %1$s", TOSTRING(OPENSCAD_VERSION));
  exit(0);
}

static int info()
{
  std::cout << LibraryInfo::info() << "\n\n";

  try {
    OffscreenView glview(512, 512);
    std::cout << glview.getRendererInfo() << "\n";
  } catch (const OffscreenViewException &ex) {
    LOG("Can't create OpenGL OffscreenView: %1$s. Exiting.\n", ex.what());
    return 1;
  }

  return 0;
}

template <typename F>
static bool with_output(const bool is_stdout, const std::string& filename, F f, std::ios::openmode mode = std::ios::out)
{
  if (is_stdout) {
#ifdef _WIN32
    if ((mode& std::ios::binary) != 0) {
      _setmode(_fileno(stdout), _O_BINARY);
    }
#endif
    f(std::cout);
    return true;
  }
  std::ofstream fstream(filename, mode);
  if (!fstream.is_open()) {
    LOG("Can't open file \"%1$s\" for export", filename);
    return false;
  } else {
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
    LOG("Could not initialize localization.");
  }
}

Camera get_camera(const po::variables_map& vm)
{
  Camera camera;

  if (vm.count("camera")) {
    vector<string> strs;
    vector<double> cam_parameters;
    boost::split(strs, vm["camera"].as<string>(), is_any_of(","));
    if (strs.size() == 6 || strs.size() == 7) {
      try {
        for (const auto& s : strs) {
          cam_parameters.push_back(lexical_cast<double>(s));
        }
        camera.setup(cam_parameters);
      } catch (bad_lexical_cast&) {
        LOG("Camera setup requires numbers as parameters");
      }
    } else {
      LOG("Camera setup requires either 7 numbers for Gimbal Camera or 6 numbers for Vector Camera");
      exit(1);
    }
  } else {
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
    } else if (proj == "p" || proj == "perspective") {
      camera.projection = Camera::ProjectionType::PERSPECTIVE;
    } else {
      LOG("projection needs to be 'o' or 'p' for ortho or perspective\n");
      exit(1);
    }
  }

  auto w = RenderSettings::inst()->img_width;
  auto h = RenderSettings::inst()->img_height;
  if (vm.count("imgsize")) {
    vector<string> strs;
    boost::split(strs, vm["imgsize"].as<string>(), is_any_of(","));
    if (strs.size() != 2) {
      LOG("Need 2 numbers for imgsize");
      exit(1);
    } else {
      try {
        w = lexical_cast<int>(strs[0]);
        h = lexical_cast<int>(strs[1]);
      } catch (bad_lexical_cast&) {
        LOG("Need 2 numbers for imgsize");
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
static bool checkAndExport(const shared_ptr<const Geometry>& root_geom, unsigned nd,
                           FileFormat format, const bool is_stdout, const std::string& filename)
{
  if (root_geom->getDimension() != nd) {
    LOG("Current top level object is not a %1$dD object.", nd);
    return false;
  }
  if (root_geom->isEmpty()) {
    LOG("Current top level object is empty.");
    return false;
  }

  ExportInfo exportInfo;
  exportInfo.format = format;
  exportInfo.name2open = filename;
  exportInfo.name2display = filename;
  exportInfo.useStdOut = is_stdout;

  exportFileByName(root_geom, exportInfo);
  return true;
}

void set_render_color_scheme(const std::string& color_scheme, const bool exit_if_not_found)
{
  if (color_scheme.empty()) {
    return;
  }

  if (ColorMap::inst()->findColorScheme(color_scheme)) {
    RenderSettings::inst()->colorscheme = color_scheme;
    return;
  }

  if (exit_if_not_found) {
    LOG((boost::algorithm::join(ColorMap::inst()->colorSchemeNames(), "\n")));

    exit(1);
  } else {
    LOG("Unknown color scheme '%1$s', using default '%2$s'.", arg_colorscheme, ColorMap::inst()->defaultColorSchemeName());
  }
}

struct CommandLine
{
  const bool is_stdin;
  const std::string& filename;
  const bool is_stdout;
  std::string output_file;
  const fs::path& original_path;
  const std::string& parameterFile;
  const std::string& setName;
  const ViewOptions& viewOptions;
  const Camera& camera;
  const boost::optional<FileFormat> export_format;
  unsigned animate_frames;
  const std::vector<std::string> summaryOptions;
  const std::string summaryFile;
};

struct RenderVariables
{
  bool preview;
  double time;
};

int do_export(const CommandLine& cmd, const RenderVariables& render_variables, FileFormat curFormat, SourceFile *root_file);

int cmdline(const CommandLine& cmd)
{
  ExportFileFormatOptions exportFileFormatOptions;
  FileFormat export_format;

  // Determine output file format and assign it to formatName
  if (cmd.export_format.is_initialized()) {
    export_format = cmd.export_format.get();
  } else {
    // else extract format from file extension
    const auto path = fs::path(cmd.output_file);
    std::string suffix = path.has_extension() ? path.extension().generic_string().substr(1) : "";
    boost::algorithm::to_lower(suffix);
    const auto format_iter = exportFileFormatOptions.exportFileFormats.find(suffix);
    if (format_iter != exportFileFormatOptions.exportFileFormats.end()) {
      export_format = format_iter->second;
    } else {
      LOG("Either add a valid suffix or specify one using the --export-format option.");
      return 1;
    }
  }

  // Do some minimal checking of output directory before rendering (issue #432)
  auto output_dir = fs::path(cmd.output_file).parent_path();
  if (output_dir.empty()) {
    // If output_file_str has no directory prefix, set output directory to current directory.
    output_dir = fs::current_path();
  }
  if (!fs::is_directory(output_dir)) {
    LOG("\n'%1$s' is not a directory for output file %2$s - Skipping\n", output_dir.generic_string(), cmd.output_file);
    return 1;
  }

  set_render_color_scheme(arg_colorscheme, true);

  shared_ptr<Echostream> echostream;
  if (export_format == FileFormat::ECHO) {
    echostream.reset(cmd.is_stdout ? new Echostream(std::cout) : new Echostream(cmd.output_file));
  }

  std::string text;
  if (cmd.is_stdin) {
    text = std::string((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
  } else {
    std::ifstream ifs(cmd.filename);
    if (!ifs.is_open()) {
      LOG("Can't open input file '%1$s'!\n", cmd.filename);
      return 1;
    }
    handle_dep(cmd.filename);
    text = std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  }

  text += "\n\x03\n" + commandline_commands;

  SourceFile *root_file = nullptr;
  if (!parse(root_file, text, cmd.filename, cmd.filename, false)) {
    delete root_file; // parse failed
    root_file = nullptr;
  }
  if (!root_file) {
    LOG("Can't parse file '%1$s'!\n", cmd.filename);
    return 1;
  }

  // add parameter to AST
  CommentParser::collectParameters(text.c_str(), root_file);
  if (!cmd.parameterFile.empty() && !cmd.setName.empty()) {
    ParameterObjects parameters = ParameterObjects::fromSourceFile(root_file);
    ParameterSets sets;
    sets.readFile(cmd.parameterFile);
    for (const auto& set : sets) {
      if (set.name() == cmd.setName) {
        parameters.importValues(set);
        parameters.apply(root_file);
        break;
      }
    }
  }

  root_file->handleDependencies();

  RenderVariables render_variables;
  render_variables.preview = canPreview(export_format) ? (cmd.viewOptions.renderer == RenderType::OPENCSG || cmd.viewOptions.renderer == RenderType::THROWNTOGETHER) : false;

  if (cmd.animate_frames == 0) {
    render_variables.time = 0;
    return do_export(cmd, render_variables, export_format, root_file);
  } else {
    // export the requested number of animated frames
    for (unsigned frame = 0; frame < cmd.animate_frames; ++frame) {
      render_variables.time = frame * (1.0 / cmd.animate_frames);

      std::ostringstream oss;
      oss << std::setw(5) << std::setfill('0') << frame;

      auto frame_file = fs::path(cmd.output_file);
      auto extension = frame_file.extension();
      frame_file.replace_extension();
      frame_file += oss.str();
      frame_file.replace_extension(extension);
      string frame_str = frame_file.generic_string();

      LOG("Exporting %1$s...", cmd.filename);

      CommandLine frame_cmd = cmd;
      frame_cmd.output_file = frame_str;

      int r = do_export(frame_cmd, render_variables, export_format, root_file);
      if (r != 0) {
        return r;
      }
    }

    return 0;
  }
}

int do_export(const CommandLine& cmd, const RenderVariables& render_variables, FileFormat curFormat, SourceFile *root_file)
{
  auto filename_str = fs::path(cmd.output_file).generic_string();
  auto fpath = fs::absolute(fs::path(cmd.filename));
  auto fparent = fpath.parent_path();

  // set CWD relative to source file
  fs::current_path(fparent);

  EvaluationSession session{fparent.string()};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
  builtin_context->set_variable("$preview", Value(render_variables.preview));
  builtin_context->set_variable("$t", Value(render_variables.time));
#ifdef DEBUG
  PRINTDB("BuiltinContext:\n%s", builtin_context->dump());
#endif

  AbstractNode::resetIndexCounter();
  std::shared_ptr<const FileContext> file_context;
  auto absolute_root_node = root_file->instantiate(*builtin_context, &file_context);
  Camera camera = cmd.camera;
  if (file_context) {
    camera.updateView(file_context, true);
  }

  // restore CWD after module instantiation finished
  fs::current_path(cmd.original_path);

  // Do we have an explicit root node (! modifier)?
  std::shared_ptr<const AbstractNode> root_node;
  const Location *nextLocation = nullptr;
  if (!(root_node = find_root_tag(absolute_root_node, &nextLocation))) {
    root_node = absolute_root_node;
  }
  if (nextLocation) {
    LOG(message_group::Warning, *nextLocation, builtin_context->documentRoot(), "More than one Root Modifier (!)");
  }
  Tree tree(root_node, fparent.string());

  if (curFormat == FileFormat::CSG) {
    // https://github.com/openscad/openscad/issues/128
    // When I use the csg ouptput from the command line the paths in 'import'
    // statements become relative. But unfortunately they become relative to
    // the current working dir and neither to the location of the input nor
    // the output.
    fs::current_path(fparent); // Force exported filenames to be relative to document path
    with_output(cmd.is_stdout, filename_str, [&tree, root_node](std::ostream& stream) {
      stream << tree.getString(*root_node, "\t") << "\n";
    });
    fs::current_path(cmd.original_path);
  } else if (curFormat == FileFormat::AST) {
    fs::current_path(fparent); // Force exported filenames to be relative to document path
    with_output(cmd.is_stdout, filename_str, [root_file](std::ostream& stream) {
      stream << root_file->dump("");
    });
    fs::current_path(cmd.original_path);
  } else if (curFormat == FileFormat::PARAM) {
    with_output(cmd.is_stdout, filename_str, [&root_file, &fpath](std::ostream& stream) {
      export_param(root_file, fpath, stream);
    });
  } else if (curFormat == FileFormat::TERM) {
    CSGTreeEvaluator csgRenderer(tree);
    auto root_raw_term = csgRenderer.buildCSGTree(*root_node);
    with_output(cmd.is_stdout, filename_str, [root_raw_term](std::ostream& stream) {
      if (!root_raw_term || root_raw_term->isEmptySet()) {
        stream << "No top-level CSG object\n";
      } else {
        stream << root_raw_term->dump() << "\n";
      }
    });
  } else if (curFormat == FileFormat::ECHO) {
    // echo -> don't need to evaluate any geometry
  } else {
#ifdef ENABLE_CGAL

    // start measuring render time
    RenderStatistic renderStatistic;
    GeometryEvaluator geomevaluator(tree);
    unique_ptr<OffscreenView> glview;
    shared_ptr<const Geometry> root_geom;
    if ((curFormat == FileFormat::ECHO || curFormat == FileFormat::PNG) && (cmd.viewOptions.renderer == RenderType::OPENCSG || cmd.viewOptions.renderer == RenderType::THROWNTOGETHER)) {
      // OpenCSG or throwntogether png -> just render a preview
      glview = prepare_preview(tree, cmd.viewOptions, camera);
      if (!glview) return 1;
    } else {
      // Force creation of CGAL objects (for testing)
      root_geom = geomevaluator.evaluateGeometry(*tree.root(), true);
      if (root_geom) {
        if (cmd.viewOptions.renderer == RenderType::CGAL && root_geom->getDimension() == 3) {
          if (auto geomlist = dynamic_pointer_cast<const GeometryList>(root_geom)) {
            auto flatlist = geomlist->flatten();
            for (auto& child : flatlist) {
              if (child.second->getDimension() == 3) {
                child.second = CGALUtils::getNefPolyhedronFromGeometry(child.second);
              }
            }
            root_geom.reset(new GeometryList(flatlist));
          } else {
            root_geom = CGALUtils::getNefPolyhedronFromGeometry(root_geom);
          }
          LOG("Converted to Nef polyhedron");
        }
      } else {
        root_geom.reset(new CGAL_Nef_polyhedron());
      }
    }
    if (curFormat == FileFormat::ASCIISTL ||
        curFormat == FileFormat::STL ||
        curFormat == FileFormat::OBJ ||
        curFormat == FileFormat::OFF ||
        curFormat == FileFormat::WRL ||
        curFormat == FileFormat::AMF ||
        curFormat == FileFormat::_3MF ||
        curFormat == FileFormat::NEFDBG ||
        curFormat == FileFormat::NEF3) {
      if (!checkAndExport(root_geom, 3, curFormat, cmd.is_stdout, filename_str)) {
        return 1;
      }
    }

    if (curFormat == FileFormat::DXF || curFormat == FileFormat::SVG || curFormat == FileFormat::PDF) {
      if (!checkAndExport(root_geom, 2, curFormat, cmd.is_stdout, filename_str)) {
        return 1;
      }
    }

    if (curFormat == FileFormat::PNG) {
      bool success = true;
      bool wrote = with_output(cmd.is_stdout, filename_str, [&success, &root_geom, &cmd, &camera, &glview](std::ostream& stream) {
        if (cmd.viewOptions.renderer == RenderType::CGAL || cmd.viewOptions.renderer == RenderType::GEOMETRY) {
          success = export_png(root_geom, cmd.viewOptions, camera, stream);
        } else {
          success = export_png(*glview, stream);
        }
      }, std::ios::out | std::ios::binary);
      if (!success || !wrote) {
        return 1;
      }
    }

    renderStatistic.printAll(root_geom, camera, cmd.summaryOptions, cmd.summaryFile);
#else
    LOG("OpenSCAD has been compiled without CGAL support!\n");
    return 1;
#endif // ifdef ENABLE_CGAL

  }
  return 0;
}

#ifdef OPENSCAD_QTGUI
#include <QtPlugin>
#include "MainWindow.h"
#include "OpenSCADApp.h"
#include "LaunchingScreen.h"
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
#include "Settings.h"

Q_DECLARE_METATYPE(Message);
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

  auto future = QtConcurrent::run([initializer] {
    return dialogThreadFunc(initializer);
  });
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
  reg_setting.setValue(QLatin1String("Software/Classes/OpenSCAD_File/DefaultIcon/Default"), QVariant(appPath));
}
#else
void registerDefaultIcon(const QString&) { }
#endif

int gui(vector<string>& inputFiles, const fs::path& original_path, int argc, char **argv)
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
#ifdef Q_OS_MAC
  app.setWindowIcon(QIcon(":/icon-macos.png"));
#else
  app.setWindowIcon(QIcon(":/logo.png"));
#endif

  // Other global settings
  qRegisterMetaType<Message>();
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

  set_render_color_scheme(arg_colorscheme, false);
  auto noInputFiles = false;

  if (!inputFiles.size()) {
    noInputFiles = true;
    inputFiles.emplace_back("");
  }

  auto showOnStartup = settings.value("launcher/showOnStartup");
  if (noInputFiles && (showOnStartup.isNull() || showOnStartup.toBool())) {
    auto launcher = new LaunchingScreen();
    auto dialogResult = launcher->exec();
    if (dialogResult == QDialog::Accepted) {
      if (launcher->isForceShowEditor()) {
        settings.setValue("view/hideEditor", false);
      }
      auto files = launcher->selectedFiles();
      // If nothing is selected in the launching screen, leave
      // the "" dummy in inputFiles to open an empty MainWindow.
      if (!files.empty()) {
        inputFiles.clear();
        for (const auto& f : files) {
          inputFiles.push_back(f.toStdString());
        }
      }
      delete launcher;
    } else {
      return 0;
    }
  }

  QStringList inputFilesList;
  for (const auto& infile: inputFiles) {
    inputFilesList.append(assemblePath(original_path, infile));
  }
  new MainWindow(inputFilesList);
  app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(releaseQSettingsCached()));
  app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

#ifdef ENABLE_HIDAPI
  if (Settings::Settings::inputEnableDriverHIDAPI.value()) {
    auto hidApi = new HidApiInputDriver();
    InputDriverManager::instance()->registerDriver(hidApi);
  }
#endif
#ifdef ENABLE_SPNAV
  if (Settings::Settings::inputEnableDriverSPNAV.value()) {
    auto spaceNavDriver = new SpaceNavInputDriver();
    bool spaceNavDominantAxisOnly = Settings::Settings::inputEnableDriverHIDAPI.value();
    spaceNavDriver->setDominantAxisOnly(spaceNavDominantAxisOnly);
    InputDriverManager::instance()->registerDriver(spaceNavDriver);
  }
#endif
#ifdef ENABLE_JOYSTICK
  if (Settings::Settings::inputEnableDriverJOYSTICK.value()) {
    std::string nr = STR(Settings::Settings::joystickNr.value());
    auto joyDriver = new JoystickInputDriver();
    joyDriver->setJoystickNr(nr);
    InputDriverManager::instance()->registerDriver(joyDriver);
  }
#endif
#ifdef ENABLE_QGAMEPAD
  if (Settings::Settings::inputEnableDriverQGAMEPAD.value()) {
    auto qGamepadDriver = new QGamepadInputDriver();
    InputDriverManager::instance()->registerDriver(qGamepadDriver);
  }
#endif
#ifdef ENABLE_DBUS
  if (Feature::ExperimentalInputDriverDBus.is_enabled()) {
    if (Settings::Settings::inputEnableDriverDBUS.value()) {
      auto dBusDriver = new DBusInputDriver();
      InputDriverManager::instance()->registerDriver(dBusDriver);
    }
  }
#endif

  InputDriverManager::instance()->init();
  int rc = app.exec();
  const auto& windows = scadApp->windowManager.getWindows();
  while (!windows.empty()) delete *windows.begin();
  return rc;
}
#else // OPENSCAD_QTGUI
bool QtUseGUI() { return false; }
int gui(const vector<string>& inputFiles, const fs::path& original_path, int argc, char **argv)
{
  LOG(message_group::Error, "Compiled without QT, but trying to run GUI\n");
  return 1;
}
#endif // OPENSCAD_QTGUI

#if defined(Q_OS_MACX)
std::pair<string, string> customSyntax(const string& s)
{
  if (s.find("-psn_") == 0) return {"psn", s.substr(5)};
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

  friend std::istream& operator>>(std::istream& in, CommaSeparatedVector& value) {
    std::string token;
    in >> token;
    // NOLINTNEXTLINE(*NewDeleteLeaks) LLVM bug https://github.com/llvm/llvm-project/issues/40486
    boost::split(value.values, token, boost::is_any_of(","));
    return in;
  }
};

template <class Seq, typename ToString>
std::string str_join(const Seq& seq, const std::string& sep, const ToString& toString)
{
  return boost::algorithm::join(boost::adaptors::transform(seq, toString), sep);
}

bool flagConvert(const std::string& str){
  if (str == "1" || boost::iequals(str, "on") || boost::iequals(str, "true")) {
    return true;
  }
  if (str == "0" || boost::iequals(str, "off") || boost::iequals(str, "false")) {
    return false;
  }
  throw std::runtime_error("");
  return false;
}

// OpenSCAD
int main(int argc, char **argv)
{
#if defined(ENABLE_CGAL) && defined(USE_MIMALLOC)
  // call init_mimalloc before any GMP variables are initialized. (defined in src/openscad_mimalloc.h)
  init_mimalloc();
#endif

  int rc = 0;
  StackCheck::inst();

#ifdef OPENSCAD_QTGUI
  { // Need a dummy app instance to get the application path but it needs to be destroyed before the GUI is launched.
    QCoreApplication app(argc, argv);
    PlatformUtils::registerApplicationPath(app.applicationDirPath().toLocal8Bit().constData());
  }
#else
  PlatformUtils::registerApplicationPath(fs::absolute(boost::filesystem::path(argv[0]).parent_path()).generic_string());
#endif

#ifdef Q_OS_MAC
  bool isGuiLaunched = getenv("GUI_LAUNCHED") != nullptr;
  auto nslog = [](const Message& msg, void *userdata) {
      CocoaUtils::nslog(msg.msg, userdata);
    };
  if (isGuiLaunched) set_output_handler(nslog, nullptr, nullptr);
#else
  PlatformUtils::ensureStdIO();
#endif

#ifdef ENABLE_CGAL
  // Always throw exceptions from CGAL, so we can catch instead of crashing on bad geometry.
  CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
  CGAL::set_warning_behaviour(CGAL::THROW_EXCEPTION);
#endif
  Builtins::instance()->initialize();

  auto original_path = fs::current_path();

  vector<string> output_files;
  const char *deps_output_file = nullptr;
  boost::optional<FileFormat> export_format;

  ViewOptions viewOptions{};
  po::options_description desc("Allowed options");
  desc.add_options()
    ("export-format", po::value<string>(), "overrides format of exported scad file when using option '-o', arg can be any of its supported file extensions.  For ascii stl export, specify 'asciistl', and for binary stl export, specify 'binstl'.  Ascii export is the current stl default, but binary stl is planned as the future default so asciistl should be explicitly specified in scripts when needed.\n")
    ("o,o", po::value<vector<string>>(), "output specified file instead of running the GUI, the file extension specifies the type: stl, off, wrl, amf, 3mf, csg, dxf, svg, pdf, png, echo, ast, term, nef3, nefdbg (May be used multiple time for different exports). Use '-' for stdout\n")
    ("D,D", po::value<vector<string>>(), "var=val -pre-define variables")
    ("p,p", po::value<string>(), "customizer parameter file")
    ("P,P", po::value<string>(), "customizer parameter set")
#ifdef ENABLE_EXPERIMENTAL
  ("enable", po::value<vector<string>>(), ("enable experimental features (specify 'all' for enabling all available features): " +
                                           str_join(boost::make_iterator_range(Feature::begin(), Feature::end()), " | ",
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
    ("animate", po::value<unsigned>(), "export N animated frames")
    ("view", po::value<CommaSeparatedVector>(), ("=view options: " + boost::algorithm::join(viewOptions.names(), " | ")).c_str())
    ("projection", po::value<string>(), "=(o)rtho or (p)erspective when exporting png")
    ("csglimit", po::value<unsigned int>(), "=n -stop rendering at n CSG elements when exporting png")
    ("summary", po::value<vector<string>>(), "enable additional render summary and statistics: all | cache | time | camera | geometry | bounding-box | area")
    ("summary-file", po::value<string>(), "output summary information in JSON format to the given file, using '-' outputs to stdout")
    ("colorscheme", po::value<string>(), ("=colorscheme: " +
                                          str_join(ColorMap::inst()->colorSchemeNames(), " | ",
                                                   [](const std::string& colorScheme) {
    return (colorScheme == ColorMap::inst()->defaultColorSchemeName() ? "*" : "") + colorScheme;
  }) +
                                          "\n").c_str())
    ("d,d", po::value<string>(), "deps_file -generate a dependency file for make")
    ("m,m", po::value<string>(), "make_cmd -runs make_cmd file if file is missing")
    ("quiet,q", "quiet mode (don't print anything *except* errors)")
    ("hardwarnings", "Stop on the first warning")
    ("trace-depth", po::value<unsigned int>(), "=n, maximum number of trace messages")
    ("trace-usermodule-parameters", po::value<string>(), "=true/false, configure the output of user module parameters in a trace")
    ("check-parameters", po::value<string>(), "=true/false, configure the parameter check for user modules and functions")
    ("check-parameter-ranges", po::value<string>(), "=true/false, configure the parameter range check for builtin modules")
    ("debug", po::value<string>(), "special debug info - specify 'all' or a set of source file names")
    ("s,s", po::value<string>(), "stl_file deprecated, use -o")
    ("x,x", po::value<string>(), "dxf_file deprecated, use -o")
  ;

  po::options_description hidden("Hidden options");
  hidden.add_options()
#ifdef Q_OS_MACX
  ("psn", po::value<string>(), "process serial number")
#endif
  ("input-file", po::value<vector<string>>(), "input file");

  po::positional_options_description p;
  p.add("input-file", -1);

  po::options_description all_options;
  all_options.add(desc).add(hidden);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(all_options).positional(p).extra_parser(customSyntax).run(), vm);
  } catch (const std::exception& e) { // Catches e.g. unknown options
    LOG("%1$s\n", e.what());
    help(argv[0], desc, true);
  }

  OpenSCAD::debug = "";
  if (vm.count("debug")) {
    OpenSCAD::debug = vm["debug"].as<string>();
    LOG("Debug on. --debug=%1$s", OpenSCAD::debug);
  }
  if (vm.count("quiet")) {
    OpenSCAD::quiet = true;
  }

  if (vm.count("hardwarnings")) {
    OpenSCAD::hardwarnings = true;
  }

  if (vm.count("traceDepth")) {
    OpenSCAD::traceDepth = vm["traceDepth"].as<unsigned int>();
  }
  std::map<std::string, bool *> flags;
  flags.insert(std::make_pair("trace-usermodule-parameters", &OpenSCAD::traceUsermoduleParameters));
  flags.insert(std::make_pair("check-parameters", &OpenSCAD::parameterCheck));
  flags.insert(std::make_pair("check-parameter-ranges", &OpenSCAD::rangeCheck));
  for (const auto& flag : flags) {
    std::string name = flag.first;
    if (vm.count(name)) {
      std::string opt = vm[name].as<string>();
      try {
        (*(flag.second) = flagConvert(opt));
      } catch (const std::runtime_error& e) {
        LOG("Could not parse '--%1$s %2$s' as flag", name, opt);
      }
    }
  }

  if (vm.count("help")) help(argv[0], desc);
  if (vm.count("version")) version();
  if (vm.count("info")) arg_info = true;

  if (vm.count("preview")) {
    if (vm["preview"].as<string>() == "throwntogether") viewOptions.renderer = RenderType::THROWNTOGETHER;
  } else if (vm.count("render")) {
    if (vm["render"].as<string>() == "cgal") viewOptions.renderer = RenderType::CGAL;
    else viewOptions.renderer = RenderType::GEOMETRY;
  }

  viewOptions.previewer = (viewOptions.renderer == RenderType::THROWNTOGETHER) ? Previewer::THROWNTOGETHER : Previewer::OPENCSG;
  if (vm.count("view")) {
    const auto& viewOptionValues = vm["view"].as<CommaSeparatedVector>();

    for (const auto& option : viewOptionValues.values) {
      try {
        viewOptions[option] = true;
      } catch (const std::out_of_range& e) {
        LOG("Unknown --view option '%1$s' ignored. Use -h to list available options.", option);
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
    LOG(message_group::Deprecated, "The -s option is deprecated. Use -o instead.\n");
    output_files.push_back(vm["s"].as<string>());
  }
  if (vm.count("x")) {
    LOG(message_group::Deprecated, "The -x option is deprecated. Use -o instead.\n");
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
    for (const auto& cmd : vm["D"].as<vector<string>>()) {
      commandline_commands += cmd;
      commandline_commands += ";\n";
    }
  }
  if (vm.count("enable")) {
    for (const auto& feature : vm["enable"].as<vector<string>>()) {
      if (feature == "all") {
        Feature::enable_all();
        break;
      }
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
  if (vm.count("input-file")) {
    inputFiles = vm["input-file"].as<vector<string>>();
  }

  if (vm.count("colorscheme")) {
    arg_colorscheme = vm["colorscheme"].as<string>();
  }

  ExportFileFormatOptions exportFileFormatOptions;
  if (vm.count("export-format")) {
    const auto format = vm["export-format"].as<string>();
    const auto format_iter = exportFileFormatOptions.exportFileFormats.find(format);
    if (format_iter != exportFileFormatOptions.exportFileFormats.end()) {
      export_format.emplace(format_iter->second);
    } else {
      LOG("Unknown --export-format option '%1$s'.  Use -h to list available options.", format);
      return 1;
    }
  }

  unsigned animate_frames = 0;
  if (vm.count("animate")) {
    animate_frames = vm["animate"].as<unsigned>();
  }

  Camera camera = get_camera(vm);

  if (animate_frames) {
    for (const auto& filename : output_files) {
      if (filename == "-") {
        LOG("Option --animate is not supported when exporting to stdout.");
        return 1;
      }
    }
    if (output_files.empty()) {
      output_files.emplace_back("frame.png");
    }
  }

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
      } else {
        for (const auto& filename : output_files) {
          const bool is_stdin = inputFiles[0] == "-";
          const std::string input_file = is_stdin ? "<stdin>" : inputFiles[0];
          const bool is_stdout = filename == "-";
          const std::string output_file = is_stdout ? "<stdout>" : filename;
          const CommandLine cmd{
            is_stdin,
            input_file,
            is_stdout,
            output_file,
            original_path,
            parameterFile,
            parameterSet,
            viewOptions,
            camera,
            export_format,
            animate_frames,
            vm.count("summary") ? vm["summary"].as<std::vector<std::string>>() : std::vector<std::string>{},
            vm.count("summary-file") ? vm["summary-file"].as<std::string>() : ""
          };
          rc |= cmdline(cmd);
        }
      }
    } catch (const HardWarningException&) {
      rc = 1;
    }

    if (deps_output_file) {
      std::string deps_out(deps_output_file);
      const vector<std::string>& geom_out(output_files);
      int result = write_deps(deps_out, geom_out);
      if (!result) {
        LOG("Error writing deps");
        return 1;
      }
    }
  } else if (QtUseGUI()) {
    if (vm.count("export-format")) {
      LOG("Ignoring --export-format option");
    }
    rc = gui(inputFiles, original_path, argc, argv);
  } else {
    LOG("Requested GUI mode but can't open display!\n");
    return 1;
  }

  Builtins::instance(true);

  return rc;
}
