#pragma once

#include <iterator>
#include <map>
#include <iostream>
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
  ASCII_STL,
  BINARY_STL,
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

struct FileFormatInfo {
  FileFormat format;
  std::string identifier;
  std::string suffix;
  std::string description;
};

namespace fileformat {

std::vector<FileFormat> all();
std::vector<FileFormat> all2D();
std::vector<FileFormat> all3D();

const FileFormatInfo& info(FileFormat fileFormat);
bool fromIdentifier(const std::string& identifier, FileFormat& format);
const std::string& toSuffix(FileFormat format);
bool canPreview(FileFormat format);
bool is3D(FileFormat format);
bool is2D(FileFormat format);

}  // namespace FileFormat

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
    bool showScale = true;
    bool showScaleMsg = true;
    bool showGrid = false;
    double gridSize = 10.0;
    bool showDesignFilename = false;
    paperOrientations Orientation = paperOrientations::PORTRAIT;
    paperSizes paperSize = paperSizes::A4;
};

struct ExportInfo {
  FileFormat format;
  std::string sourceFilePath; // Full path to the OpenSCAD source file
  ExportPdfOptions *options;
  const Camera *camera;
};

bool exportFileByName(const std::shared_ptr<const class Geometry>& root_geom, const std::string& filename, const ExportInfo& exportInfo);
bool exportFileStdOut(const std::shared_ptr<const class Geometry>& root_geom, const ExportInfo& exportInfo);

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
