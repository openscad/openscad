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

#include <chrono>
#include <iomanip>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/bind/bind.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/program_options.hpp>
#include <filesystem>
#include <boost/optional.hpp>
#include <boost/dll.hpp>

#ifdef ENABLE_CGAL
#include <CGAL/assertions.h>
#endif

#include "core/Builtins.h"
#include "core/CSGTreeEvaluator.h"
#include "core/customizer/CommentParser.h"
#include "core/customizer/ParameterObject.h"
#include "core/customizer/ParameterSet.h"
#include "core/parsersettings.h"
#include "core/RenderVariables.h"
#include "geometry/GeometryEvaluator.h"
#include "glview/ColorMap.h"
#include "glview/OffscreenView.h"
#include "glview/RenderSettings.h"
#include "handle_dep.h"
#include "io/export.h"
#include "LibraryInfo.h"
#include "openscad_gui.h"
#include "openscad_mimalloc.h"
#include "platform/PlatformUtils.h"
#include "RenderStatistic.h"
#include "utils/StackCheck.h"
#include "printutils.h"


#ifdef ENABLE_PYTHON
extern std::shared_ptr<AbstractNode> python_result_node;
std::string evaluatePython(const std::string &code, double time);
bool python_active = false;
bool python_trusted = false;
#endif
namespace po = boost::program_options;
namespace fs = std::filesystem;

std::string commandline_commands;
static bool arg_info = false;
std::string arg_colorscheme;

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

namespace {

#ifndef OPENSCAD_NOGUI
bool useGUI()
{
#ifdef Q_OS_X11
  // see <http://qt.nokia.com/doc/4.5/qapplication.html#QApplication-2>:
  // On X11, the window system is initialized if GUIenabled is true. If GUIenabled
  // is false, the application does not connect to the X server. On Windows and
  // Macintosh, currently the window system is always initialized, regardless of the
  // value of GUIenabled. This may change in future versions of Qt.
  return getenv("DISPLAY") != 0;
#else
  return true;
#endif
}
#endif // OPENSCAD_NOGUI

bool checkAndExport(const std::shared_ptr<const Geometry>& root_geom, unsigned dimensions,
                    FileFormat format, const bool is_stdout, const std::string& filename,
		    const Camera *const camera, const std::string& input_filename)
{
  if (root_geom->getDimension() != dimensions) {
    LOG("Current top level object is not a %1$dD object.", dimensions);
    return false;
  }
  if (root_geom->isEmpty()) {
    LOG("Current top level object is empty.");
    return false;
  }
  ExportInfo exportInfo = {.format = format, .sourceFilePath = input_filename, .camera = camera};
  if (is_stdout) {
    exportFileStdOut(root_geom, exportInfo);
  }
  else {
    exportFileByName(root_geom, filename, exportInfo);
  }
  return true;
}

void help(const char *arg0, const po::options_description& desc, bool failure = false)
{
  const fs::path progpath(arg0);
  LOG("Usage: %1$s [options] file.scad\n%2$s", progpath.filename().string(), desc);
  exit(failure ? 1 : 0);
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
void version()
{
  LOG("OpenSCAD version %1$s", TOSTRING(OPENSCAD_VERSION));
  exit(0);
}

int info()
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
bool with_output(const bool is_stdout, const std::string& filename, F f, std::ios::openmode mode = std::ios::out)
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

} // namespace

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

/**
 * Initialize gettext. This must be called after the application path was
 * determined so we can lookup the resource path for the language translation
 * files.
 */
void localization_init() {
  fs::path po_dir(PlatformUtils::resourcePath("locale"));
  const std::string& locale_path(po_dir.string());

  if (fs::is_directory(locale_path)) {
    setlocale(LC_ALL, "");
    bindtextdomain("openscad", locale_path.c_str());
    bind_textdomain_codeset("openscad", "UTF-8");
    textdomain("openscad");
  } else {
    LOG("Could not initialize localization (application path is '%1$s').", PlatformUtils::applicationPath());
  }
}

struct AnimateArgs {
  unsigned frames = 0;
  unsigned num_shards = 1;
  unsigned shard = 1;
};

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
  const AnimateArgs animate;
  const std::vector<std::string> summaryOptions;
  const std::string summaryFile;
};

AnimateArgs get_animate(const po::variables_map& vm) {
  AnimateArgs animate;
  if (vm.count("animate")) {
    animate.frames = vm["animate"].as<unsigned>();
  }
  if (vm.count("animate_sharding")) {
    std::vector<std::string> strs;
    boost::split(strs, vm["animate_sharding"].as<std::string>(),
                 boost::is_any_of("/"));
    if (strs.size() != 2) {
      LOG("--animate_sharding requires <shard>/<num_shards>");
      exit(1);
    }
    try {
      animate.shard = boost::lexical_cast<unsigned>(strs[0]);
      animate.num_shards = boost::lexical_cast<unsigned>(strs[1]);
    } catch (const boost::bad_lexical_cast&) {
      LOG("--animate_sharding parameters need to be positive integers");
      exit(1);
    }
    if (animate.shard > animate.num_shards || animate.shard == 0) {
      LOG("--animate_sharding: shard needs to be in range <1..num_shards>");
      exit(1);
    }
  }
  return animate;
}

Camera get_camera(const po::variables_map& vm)
{
  Camera camera;

  if (vm.count("camera")) {
    std::vector<std::string> strs;
    std::vector<double> cam_parameters;
    boost::split(strs, vm["camera"].as<std::string>(), boost::is_any_of(","));
    if (strs.size() == 6 || strs.size() == 7) {
      try {
        for (const auto& s : strs) {
          cam_parameters.push_back(boost::lexical_cast<double>(s));
        }
        camera.setup(cam_parameters);
      } catch (boost::bad_lexical_cast&) {
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
    auto proj = vm["projection"].as<std::string>();
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
    std::vector<std::string> strs;
    boost::split(strs, vm["imgsize"].as<std::string>(), boost::is_any_of(","));
    if (strs.size() != 2) {
      LOG("Need 2 numbers for imgsize");
      exit(1);
    } else {
      try {
        w = boost::lexical_cast<int>(strs[0]);
        h = boost::lexical_cast<int>(strs[1]);
      } catch (boost::bad_lexical_cast&) {
        LOG("Need 2 numbers for imgsize");
      }
    }
  }
  camera.pixel_width = w;
  camera.pixel_height = h;

  return camera;
}

int do_export(const CommandLine& cmd, const RenderVariables& render_variables, FileFormat export_format, SourceFile *root_file)
{
  auto filename_str = fs::path(cmd.output_file).generic_string();
  // Avoid possibility of fs::absolute throwing when passed an empty path
  auto fpath = cmd.filename.empty() ? fs::current_path() : fs::absolute(fs::path(cmd.filename));
  auto fparent = fpath.parent_path();

  // set CWD relative to source file
  fs::current_path(fparent);

  EvaluationSession session{fparent.string()};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
  render_variables.applyToContext(builtin_context);

#ifdef DEBUG
  PRINTDB("BuiltinContext:\n%s", builtin_context->dump());
#endif

  AbstractNode::resetIndexCounter();
  std::shared_ptr<const FileContext> file_context;
  std::shared_ptr<AbstractNode> absolute_root_node;

#ifdef ENABLE_PYTHON    
  if(python_result_node != NULL && python_active) {
    absolute_root_node = python_result_node;
  } else {
#endif	    
  absolute_root_node = root_file->instantiate(*builtin_context, &file_context);
#ifdef ENABLE_PYTHON
  }
#endif

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

  if (export_format == FileFormat::CSG) {
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
  } else if (export_format == FileFormat::AST) {
    fs::current_path(fparent); // Force exported filenames to be relative to document path
    with_output(cmd.is_stdout, filename_str, [root_file](std::ostream& stream) {
      stream << root_file->dump("");
    });
    fs::current_path(cmd.original_path);
  } else if (export_format == FileFormat::PARAM) {
    with_output(cmd.is_stdout, filename_str, [&root_file, &fpath](std::ostream& stream) {
      export_param(root_file, fpath, stream);
    });
  } else if (export_format == FileFormat::TERM) {
    CSGTreeEvaluator csgRenderer(tree);
    auto root_raw_term = csgRenderer.buildCSGTree(*root_node);
    with_output(cmd.is_stdout, filename_str, [root_raw_term](std::ostream& stream) {
      if (!root_raw_term || root_raw_term->isEmptySet()) {
        stream << "No top-level CSG object\n";
      } else {
        stream << root_raw_term->dump() << "\n";
      }
    });
  } else if (export_format == FileFormat::ECHO) {
    // echo -> don't need to evaluate any geometry
  } else {
    // start measuring render time
    RenderStatistic renderStatistic;
    GeometryEvaluator geomevaluator(tree);
    std::unique_ptr<OffscreenView> glview;
    std::shared_ptr<const Geometry> root_geom;
    if ((export_format == FileFormat::ECHO || export_format == FileFormat::PNG) && (cmd.viewOptions.renderer == RenderType::OPENCSG || cmd.viewOptions.renderer == RenderType::THROWNTOGETHER)) {
      // OpenCSG or throwntogether png -> just render a preview
      glview = prepare_preview(tree, cmd.viewOptions, camera);
      if (!glview) return 1;
    } else {
      // Force creation of concrete geometry (mostly for testing)
      // FIXME: Consider adding MANIFOLD as a valid --render argument and ViewOption, to be able to distinguish from CGAL

      constexpr bool allownef = true;
      root_geom = geomevaluator.evaluateGeometry(*tree.root(), allownef);
      if (!root_geom) root_geom = std::make_shared<PolySet>(3);
      if (cmd.viewOptions.renderer == RenderType::BACKEND_SPECIFIC && root_geom->getDimension() == 3) {
        if (auto geomlist = std::dynamic_pointer_cast<const GeometryList>(root_geom)) {
          auto flatlist = geomlist->flatten();
          for (auto& child : flatlist) {
            if (child.second->getDimension() == 3) {
              child.second = GeometryUtils::getBackendSpecificGeometry(child.second);
            }
          }
          root_geom = std::make_shared<GeometryList>(flatlist);
        } else {
          root_geom = GeometryUtils::getBackendSpecificGeometry(root_geom);
        }
        LOG("Converted to backend-specific geometry");
      }
    }

    const std::string input_filename = cmd.is_stdin ? "<stdin>" : cmd.filename;
    const int dim = fileformat::is3D(export_format) ? 3 : fileformat::is2D(export_format) ? 2 : 0;
    if (dim > 0 && !checkAndExport(root_geom, dim, export_format, cmd.is_stdout, filename_str, &cmd.camera, input_filename)) {
      return 1;
    }

    if (export_format == FileFormat::PNG) {
      bool success = true;
      bool wrote = with_output(cmd.is_stdout, filename_str, [&success, &root_geom, &cmd, &camera, &glview](std::ostream& stream) {
        if (cmd.viewOptions.renderer == RenderType::BACKEND_SPECIFIC || cmd.viewOptions.renderer == RenderType::GEOMETRY) {
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
  }
  return 0;
}

int cmdline(const CommandLine& cmd)
{
  FileFormat export_format;

  // Determine output file format and assign it to formatName
  if (cmd.export_format.is_initialized()) {
    export_format = cmd.export_format.get();
  } else {
    // else extract format from file extension
    const auto path = fs::path(cmd.output_file);
    std::string suffix = path.has_extension() ? path.extension().generic_string().substr(1) : "";
    boost::algorithm::to_lower(suffix);

    if (!fileformat::fromIdentifier(suffix, export_format)) {
      LOG("Invalid suffix %1$s. Either add a valid suffix or specify one using the --export-format option.", suffix);
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

  std::shared_ptr<Echostream> echostream;
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

#ifdef ENABLE_PYTHON  
  python_active = false;
  if(cmd.filename.c_str() != NULL) {
	  if(boost::algorithm::ends_with(cmd.filename, ".py")) {
		  if( python_trusted == true) python_active = true;
		  else  LOG("Python is not enabled");
	  }
  }

  if(python_active) {
    auto fulltext_py = text;
    auto error  = evaluatePython(fulltext_py, 0.0);
    if(error.size() > 0) LOG(error.c_str());
    text ="\n";
  }
#endif	  
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

  RenderVariables render_variables = {
    .preview = fileformat::canPreview(export_format)
      ? (cmd.viewOptions.renderer == RenderType::OPENCSG
        || cmd.viewOptions.renderer == RenderType::THROWNTOGETHER)
      : false,
    .camera = cmd.camera,
  };

  if (cmd.animate.frames == 0) {
    render_variables.time = 0;
    return do_export(cmd, render_variables, export_format, root_file);
  } else {
    // export the requested number of animated frames
    const unsigned start_frame = ((cmd.animate.shard - 1) * cmd.animate.frames)
      / cmd.animate.num_shards;
    const unsigned limit_frame = (cmd.animate.shard * cmd.animate.frames)
      / cmd.animate.num_shards;
    for (unsigned frame = start_frame; frame < limit_frame; ++frame) {
      render_variables.time = frame * (1.0 / cmd.animate.frames);

      std::ostringstream oss;
      oss << std::setw(5) << std::setfill('0') << frame;

      auto frame_file = fs::path(cmd.output_file);
      auto extension = frame_file.extension();
      frame_file.replace_extension();
      frame_file += oss.str();
      frame_file.replace_extension(extension);
      std::string frame_str = frame_file.generic_string();

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

#ifdef Q_OS_MACOS
std::pair<std::string, std::string> customSyntax(const std::string& s)
{
  if (s.find("-psn_") == 0) return {"psn", s.substr(5)};
#else
std::pair<std::string, std::string> customSyntax(const std::string&)
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

#ifdef Q_OS_MACOS
  bool isGuiLaunched = getenv("GUI_LAUNCHED") != nullptr;
  auto nslog = [](const Message& msg, void *userdata) {
      CocoaUtils::nslog(msg.msg, userdata);
    };
  if (isGuiLaunched) set_output_handler(nslog, nullptr, nullptr);
#else
  PlatformUtils::ensureStdIO();
#endif

  const auto applicationPath = weakly_canonical(boost::dll::program_location()).parent_path().generic_string();
  PlatformUtils::registerApplicationPath(applicationPath);

#ifdef ENABLE_CGAL
  // Always throw exceptions from CGAL, so we can catch instead of crashing on bad geometry.
  CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
  CGAL::set_warning_behaviour(CGAL::THROW_EXCEPTION);
#endif
  Builtins::instance()->initialize();

  auto original_path = fs::current_path();

  std::vector<std::string> output_files;
  const char *deps_output_file = nullptr;
  boost::optional<FileFormat> export_format;

  ViewOptions viewOptions{};
  po::options_description desc("Allowed options");
  desc.add_options()
    ("export-format", po::value<std::string>(), "overrides format of exported scad file when using option '-o', arg can be any of its supported file extensions.  For ascii stl export, specify 'asciistl', and for binary stl export, specify 'binstl'.  Ascii export is the current stl default, but binary stl is planned as the future default so asciistl should be explicitly specified in scripts when needed.\n")
    ("o,o", po::value<std::vector<std::string>>(), "output specified file instead of running the GUI, the file extension specifies the type: stl, off, wrl, amf, 3mf, csg, dxf, svg, pdf, png, echo, ast, term, nef3, nefdbg (May be used multiple time for different exports). Use '-' for stdout\n")
    ("D,D", po::value<std::vector<std::string>>(), "var=val -pre-define variables")
    ("p,p", po::value<std::string>(), "customizer parameter file")
    ("P,P", po::value<std::string>(), "customizer parameter set")
#ifdef ENABLE_EXPERIMENTAL
  ("enable", po::value<std::vector<std::string>>(), ("enable experimental features (specify 'all' for enabling all available features): " +
                                           str_join(boost::make_iterator_range(Feature::begin(), Feature::end()), " | ",
                                                    [](const Feature *feature) {
    return feature->get_name();
  }) +
                                           "\n").c_str())
#endif
  ("help,h", "print this help message and exit")
    ("version,v", "print the version")
    ("info", "print information about the build process\n")

    ("camera", po::value<std::string>(), "camera parameters when exporting png: =translate_x,y,z,rot_x,y,z,dist or =eye_x,y,z,center_x,y,z")
    ("autocenter", "adjust camera to look at object's center")
    ("viewall", "adjust camera to fit object")
    ("backend", po::value<std::string>(), "3D rendering backend to use: 'CGAL' (old/slow) [default] or 'Manifold' (new/fast)")
    ("imgsize", po::value<std::string>(), "=width,height of exported png")
    ("render", po::value<std::string>()->implicit_value(""), "for full geometry evaluation when exporting png")
    ("preview", po::value<std::string>()->implicit_value(""), "[=throwntogether] -for ThrownTogether preview png")
    ("animate", po::value<unsigned>(), "export N animated frames")
    ("animate_sharding", po::value<std::string>(), "Parameter <shard>/<num_shards> - Divide work into <num_shards> and only output frames for <shard>. E.g. 2/5 only outputs the second 1/5 of frames. Use to parallelize work on multiple cores or machines.")
    ("view", po::value<CommaSeparatedVector>(), ("=view options: " + boost::algorithm::join(viewOptions.names(), " | ")).c_str())
    ("projection", po::value<std::string>(), "=(o)rtho or (p)erspective when exporting png")
    ("csglimit", po::value<unsigned int>(), "=n -stop rendering at n CSG elements when exporting png")
    ("summary", po::value<std::vector<std::string>>(), "enable additional render summary and statistics: all | cache | time | camera | geometry | bounding-box | area")
    ("summary-file", po::value<std::string>(), "output summary information in JSON format to the given file, using '-' outputs to stdout")
    ("colorscheme", po::value<std::string>(), ("=colorscheme: " +
                                          str_join(ColorMap::inst()->colorSchemeNames(), " | ",
                                                   [](const std::string& colorScheme) {
    return (colorScheme == ColorMap::inst()->defaultColorSchemeName() ? "*" : "") + colorScheme;
  }) +
                                          "\n").c_str())
    ("d,d", po::value<std::string>(), "deps_file -generate a dependency file for make")
    ("m,m", po::value<std::string>(), "make_cmd -runs make_cmd file if file is missing")
    ("quiet,q", "quiet mode (don't print anything *except* errors)")
    ("hardwarnings", "Stop on the first warning")
    ("trace-depth", po::value<unsigned int>(), "=n, maximum number of trace messages")
    ("trace-usermodule-parameters", po::value<std::string>(), "=true/false, configure the output of user module parameters in a trace")
    ("check-parameters", po::value<std::string>(), "=true/false, configure the parameter check for user modules and functions")
    ("check-parameter-ranges", po::value<std::string>(), "=true/false, configure the parameter range check for builtin modules")
    ("debug", po::value<std::string>(), "special debug info - specify 'all' or a set of source file names")
    ("s,s", po::value<std::string>(), "stl_file deprecated, use -o")
    ("x,x", po::value<std::string>(), "dxf_file deprecated, use -o")
#ifdef ENABLE_PYTHON
  ("trust-python",  "Trust python")
#endif
  ;

  po::options_description hidden("Hidden options");
  hidden.add_options()
#ifdef Q_OS_MACOS
  ("psn", po::value<std::string>(), "process serial number")
#endif
  ("input-file", po::value<std::vector<std::string>>(), "input file");

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
    OpenSCAD::debug = vm["debug"].as<std::string>();
    LOG("Debug on. --debug=%1$s", OpenSCAD::debug);
  }
#ifdef ENABLE_PYTHON
  if (vm.count("trust-python")) {
    LOG("Python Engine enabled", OpenSCAD::debug);
    python_trusted = true;
  }
#endif
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
      std::string opt = vm[name].as<std::string>();
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
  if (vm.count("backend")) {
    RenderSettings::inst()->backend3D = renderBackend3DFromString(vm["backend"].as<std::string>());
  }

  if (vm.count("preview")) {
    if (vm["preview"].as<std::string>() == "throwntogether") viewOptions.renderer = RenderType::THROWNTOGETHER;
  } else if (vm.count("render")) {
    // Note: "cgal" is here for backwards compatibility, can probably be removed soon
    if (vm["render"].as<std::string>() == "cgal" || vm["render"].as<std::string>() == "force") {
      viewOptions.renderer = RenderType::BACKEND_SPECIFIC;
    } else {
      viewOptions.renderer = RenderType::GEOMETRY;
    }
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
    output_files = vm["o"].as<std::vector<std::string>>();
  }
  if (vm.count("s")) {
    LOG(message_group::Deprecated, "The -s option is deprecated. Use -o instead.\n");
    output_files.push_back(vm["s"].as<std::string>());
  }
  if (vm.count("x")) {
    LOG(message_group::Deprecated, "The -x option is deprecated. Use -o instead.\n");
    output_files.push_back(vm["x"].as<std::string>());
  }
  if (vm.count("d")) {
    if (deps_output_file) help(argv[0], desc, true);
    deps_output_file = vm["d"].as<std::string>().c_str();
  }
  if (vm.count("m")) {
    if (make_command) help(argv[0], desc, true);
    make_command = vm["m"].as<std::string>().c_str();
  }

  if (vm.count("D")) {
    for (const auto& cmd : vm["D"].as<std::vector<std::string>>()) {
      commandline_commands += cmd;
      commandline_commands += ";\n";
    }
  }
  if (vm.count("enable")) {
    for (const auto& feature : vm["enable"].as<std::vector<std::string>>()) {
      if (feature == "all") {
        Feature::enable_all();
        break;
      }
      Feature::enable_feature(feature);
    }
  }

  std::string parameterFile;
  if (vm.count("p")) {
    if (!parameterFile.empty()) {
      help(argv[0], desc, true);
    }
    parameterFile = vm["p"].as<std::string>().c_str();
  }

  std::string parameterSet;
  if (vm.count("P")) {
    if (!parameterSet.empty()) {
      help(argv[0], desc, true);
    }
    parameterSet = vm["P"].as<std::string>().c_str();
  }

  std::vector<std::string> inputFiles;
  if (vm.count("input-file")) {
    inputFiles = vm["input-file"].as<std::vector<std::string>>();
  }

  if (vm.count("colorscheme")) {
    arg_colorscheme = vm["colorscheme"].as<std::string>();
  }

  if (vm.count("export-format")) {
    const auto format_str = vm["export-format"].as<std::string>();
    FileFormat format;
    if (fileformat::fromIdentifier(format_str, format)) {
      export_format.emplace(format);

    } else {
      LOG("Unknown --export-format option '%1$s'.  Use -h to list available options.", format_str);
      return 1;
    }
  }

  AnimateArgs animate = get_animate(vm);
  Camera camera = get_camera(vm);

  if (animate.frames) {
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

  PRINTDB("Application location detected as %s", applicationPath);

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
            animate,
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
      const std::vector<std::string>& geom_out(output_files);
      int result = write_deps(deps_out, geom_out);
      if (!result) {
        LOG("Error writing deps");
        return 1;
      }
    }
#ifndef OPENSCAD_NOGUI
  } else if (useGUI()) {
    if (vm.count("export-format")) {
      LOG("Ignoring --export-format option");
    }
    rc = gui(inputFiles, original_path, argc, argv);
#endif
  } else {
    LOG("Requested GUI mode but can't open display!\n");
    return 1;
  }

  Builtins::instance(true);

  return rc;
}
