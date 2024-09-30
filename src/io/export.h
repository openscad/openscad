#pragma once

#include <map>
#include <iostream>
#include <functional>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

#include "core/Tree.h"
#include "glview/Camera.h"

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
  NEFDBG,
  NEF3,
  CSG,
  AST,
  TERM,
  ECHO,
  PNG,
  PDF,
  POV,
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

struct ExportInfo {
  FileFormat format;
  std::string displayName;
  std::string fileName;
  std::string sourceFilePath;
  std::string sourceFileName;
  bool useStdOut;
  ExportPdfOptions *options;
};


bool canPreview(const FileFormat format);
bool is3D(const FileFormat format);
bool is2D(const FileFormat format);

bool exportFileByName(const std::shared_ptr<const class Geometry>& root_geom, const ExportInfo& exportInfo);

void export_stl(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                bool binary = true);
void export_3mf(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_obj(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_off(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_wrl(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_amf(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_svg(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_pov(const std::shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo);
void export_pdf(const std::shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo);
void export_nefdbg(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_nef3(const std::shared_ptr<const Geometry>& geom, std::ostream& output);


enum class Previewer { OPENCSG, THROWNTOGETHER };
enum class RenderType { GEOMETRY, BACKEND_SPECIFIC, OPENCSG, THROWNTOGETHER };

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
    {"nefdbg", FileFormat::NEFDBG},
    {"nef3", FileFormat::NEF3},
    {"csg", FileFormat::CSG},
    {"param", FileFormat::PARAM},
    {"ast", FileFormat::AST},
    {"term", FileFormat::TERM},
    {"echo", FileFormat::ECHO},
    {"png", FileFormat::PNG},
    {"pdf", FileFormat::PDF},
    {"pov", FileFormat::POV},
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
bool export_png(const std::shared_ptr<const class Geometry>& root_geom, const ViewOptions& options, Camera& camera, std::ostream& output);
bool export_png(const OffscreenView& glview, std::ostream& output);
bool export_param(SourceFile *root, const fs::path& path, std::ostream& output);

std::unique_ptr<PolySet> createSortedPolySet(const PolySet& ps);
