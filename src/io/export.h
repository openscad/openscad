#pragma once

#include <iostream>
#include <functional>
#include <array>
#include <any>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

#include "Tree.h"
#include "Camera.h"
#include "memory.h"

class PolySet;

enum class FileFormat {
  ASCIISTL,
  STL,
  OBJ,
  OFF,
  WRL,
  AMF,
  _3MF,
  DXF,
  SVG,
  LBRN,
  NEFDBG,
  NEF3,
  CSG,
  AST,
  TERM,
  ECHO,
  PNG,
  PDF,
  PARAM
};


// Paper Data used by ExportPDF
enum class paperSizes {
 A4,A3,LETTER,LEGAL,TABLOID
};
// Note:  the enum could be moved to GUI, which would pass the dimensions.

// for gui, but declared here to keep it aligned with the enum.
// can't use Qt mechanism in the IO code.
// needs to match number of sizes
const std::array<std::string,5> paperSizeStrings{  
"A4","A3","Letter","Legal","Tabloid"
};


// Dimensions in pts per PDF standard, used by ExportPDF
// rows map to paperSizes enums
// columns are Width, Height
const int paperDimensions[5][2]={
{595,842},
{842,1190},
{612,792},
{612,1008},
{792,1224}
}; 

enum class paperOrientations {
PORTRAIT,LANDSCAPE,AUTO
};

// for gui, but declared here to keep it aligned with the enum.
// can't use Qt mechanism in the IO code.
// needs to match number of orientations
const std::array<std::string,3> paperOrientationsStrings{  
"Portrait","Landscape","Auto"
};

// include defaults to use without dialog or direction.
// Defaults match values used prior to incorporation of options.
struct ExportPdfOptions {
    bool showScale=TRUE;
    bool showScaleMsg=TRUE;
    bool showGrid=FALSE;
    double gridSize=10.; // New
    bool showDsgnFN=TRUE; 
    paperOrientations Orientation=paperOrientations::PORTRAIT;
    paperSizes paperSize=paperSizes::A4;
};

// include defaults to use without dialog or direction.
// Defaults match values used prior to incorporation of options.
struct ExportLbrnOptions {
  int colorIndex = 0;
  int minPower   = 40;
  int maxPower   = 40;
  int speed      = 8;
  int numPasses  = 1;
};

struct ExportLbrnOptionsx {
  int colorIndex;
  int minPower;
  int maxPower;
  int speed;
  int numPasses;
};

struct CommandExportOption {
  std::string format;
  std::string option;
  std::string value;
  std::string type;
};

CommandExportOption parseCommandExportOption(std::string *option);
CommandExportOption getExportOption(std::vector<CommandExportOption> exportOptions, std::string *format, std::string *option);
std::vector<CommandExportOption> getExportOptions(std::vector<CommandExportOption> exportOptions, std::string *format);

//extern std::map<std::string, std::any> ExportOptionsMap;
extern std::map< std::string, std::map<std::string, std::any> > ExportOptionsMap;
/*
std::map<std::string,std::string> ExportLbrnOptionsMap = {
  { "colorIndex", "0" },
  { "minPower", "40" },
  { "maxPower", "40" },
  { "speed", "8" },
  { "numPasses", "1" } 
};
*/


class ExportFileOptions {
  
  public:
    ExportFileOptions(); 
    
    bool parse_command_export_option(const std::string& option);
    bool add_option(const std::string& format, const std::string& option, const std::string& value, const std::string& type);
	bool update_option(const std::string& format, const std::string& option, const std::string& value);
    bool option_exists(const std::string& format, const std::string& option);
    int get_option_index(const std::string& format, const std::string& option);
    std::string get_option_value(const std::string& format, const std::string& option);
    CommandExportOption get_option(const std::string& format, const std::string& option);
    std::vector<CommandExportOption> get_options(const std::string& format);
    bool is_value_valid(const std::string& format, const std::string& option, const std::string& value);
    bool is_number(const std::string& str);
    bool is_float(const std::string& str);
    bool is_bool(const std::string& str);
    bool is_string(const std::string& str);

  private:
    std::vector<CommandExportOption> commandExportOptions;

};


struct ExportInfo {
  FileFormat format;
  std::string name2display;
  std::string name2open;
  std::string sourceFilePath;
  std::string sourceFileName;
  bool useStdOut;
  ExportPdfOptions *options=nullptr;
  ExportLbrnOptions *lbrnOptions=nullptr;
  ExportFileOptions exportFileOptions;
};


bool canPreview(const FileFormat format);
bool exportFileByName(const shared_ptr<const class Geometry>& root_geom, const ExportInfo& exportInfo);

void export_stl(const shared_ptr<const Geometry>& geom, std::ostream& output,
                bool binary = true);
void export_3mf(const shared_ptr<const Geometry>& geom, std::ostream& output);
void export_obj(const shared_ptr<const Geometry>& geom, std::ostream& output);
void export_off(const shared_ptr<const Geometry>& geom, std::ostream& output);
void export_wrl(const shared_ptr<const Geometry>& geom, std::ostream& output);
void export_amf(const shared_ptr<const Geometry>& geom, std::ostream& output);
void export_dxf(const shared_ptr<const Geometry>& geom, std::ostream& output);
void export_svg(const shared_ptr<const Geometry>& geom, std::ostream& output);


void export_lbrn(const shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo);
void export_pdf(const shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo);
void export_nefdbg(const shared_ptr<const Geometry>& geom, std::ostream& output);
void export_nef3(const shared_ptr<const Geometry>& geom, std::ostream& output);

enum class Previewer { OPENCSG, THROWNTOGETHER };
enum class RenderType { GEOMETRY, CGAL, OPENCSG, THROWNTOGETHER };

struct ExportFileFormatOptions {
  const std::map<const std::string, FileFormat> exportFileFormats{
    {"asciistl", FileFormat::ASCIISTL},
    {"binstl", FileFormat::STL},
    {"stl", FileFormat::ASCIISTL}, // Deprecated.  Later to FileFormat::STL
    {"obj", FileFormat::OBJ},
    {"off", FileFormat::OFF},
    {"wrl", FileFormat::WRL},
    {"amf", FileFormat::AMF},
    {"3mf", FileFormat::_3MF},
    {"dxf", FileFormat::DXF},
    {"svg", FileFormat::SVG},
    {"lbrn", FileFormat::LBRN},
    {"nefdbg", FileFormat::NEFDBG},
    {"nef3", FileFormat::NEF3},
    {"csg", FileFormat::CSG},
    {"param", FileFormat::PARAM},
    {"ast", FileFormat::AST},
    {"term", FileFormat::TERM},
    {"echo", FileFormat::ECHO},
    {"png", FileFormat::PNG},
    {"pdf", FileFormat::PDF},
  };
};

struct ViewOption {
  const std::string name;
  bool& value;
};

struct ViewOptions {
  Previewer previewer{Previewer::OPENCSG};
  RenderType renderer{RenderType::OPENCSG};

  std::map<std::string, bool> flags{
    {"axes", false},
    {"scales", false},
    {"edges", false},
    {"wireframe", false},
    {"crosshairs", false},
  };

  const std::vector<std::string> names() {
    std::vector<std::string> names;
    boost::copy(flags | boost::adaptors::map_keys, std::back_inserter(names));
    return names;
  }

  bool& operator[](const std::string& name) {
    return flags.at(name);
  }

  bool operator[](const std::string& name) const {
    return flags.at(name);
  }

};

class OffscreenView;

std::unique_ptr<OffscreenView> prepare_preview(Tree& tree, const ViewOptions& options, Camera& camera);
bool export_png(const shared_ptr<const class Geometry>& root_geom, const ViewOptions& options, Camera& camera, std::ostream& output);
bool export_png(const OffscreenView& glview, std::ostream& output);
bool export_param(SourceFile *root, const fs::path& path, std::ostream& output);

namespace Export {

struct Triangle {
  std::array<int, 3> key;
  Triangle(int p1, int p2, int p3)
  {
    // sort vertices with smallest value first without
    // changing winding order of the triangle.
    // See https://github.com/nophead/Mendel90/blob/master/c14n_stl.py

    if (p1 < p2) {
      if (p1 < p3) {
        key = {p1, p2, p3}; // v1 is the smallest
      } else {
        key = {p3, p1, p2}; // v3 is the smallest
      }
    } else {
      if (p2 < p3) {
        key = {p2, p3, p1}; // v2 is the smallest
      } else {
        key = {p3, p1, p2}; // v3 is the smallest
      }
    }
  }
};

class ExportMesh
{
public:
  using Vertex = std::array<double, 3>;

  ExportMesh(const PolySet& ps);

  bool foreach_vertex(const std::function<bool(const Vertex&)>& callback) const;
  bool foreach_indexed_triangle(const std::function<bool(const std::array<int, 3>&)>& callback) const;
  bool foreach_triangle(const std::function<bool(const std::array<Vertex, 3>&)>& callback) const;

private:
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
};

} // namespace Export
