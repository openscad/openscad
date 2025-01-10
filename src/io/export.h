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

#include "Settings.h"
#include "core/Tree.h"
#include "glview/Camera.h"
#include "glview/ColorMap.h"
#include "linalg.h"

#include "io/export_enums.h"

using S = Settings::Settings;

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

constexpr inline auto EXPORT_CREATOR = "OpenSCAD (https://www.openscad.org/)";

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

// include defaults to use without dialog or direction.
// Defaults match values used prior to incorporation of options.
struct ExportPdfOptions {
    bool showScale = true;
    bool showScaleMsg = true;
    bool showGrid = false;
    double gridSize = 10.0;
    bool showDesignFilename = false;
    PaperOrientations orientation = PaperOrientations::PORTRAIT;
    PaperSizes paperSize = PaperSizes::A4;
    bool addMetaData = S::exportPdfAddMetaData.defaultValue();
    std::string metaDataTitle;
    std::string metaDataAuthor;
    std::string metaDataSubject;
    std::string metaDataKeywords;

  static const std::shared_ptr<const ExportPdfOptions> fromSettings() {
    return std::make_shared<const ExportPdfOptions>(ExportPdfOptions{
      .showScale = S::exportPdfShowScale.value(),
      .showScaleMsg = S::exportPdfShowScaleMessage.value(),
      .showGrid = S::exportPdfShowGrid.value(),
      .gridSize = S::exportPdfGridSize.value(),
      .showDesignFilename = S::exportPdfShowFilename.value(),
      .orientation = S::exportPdfOrientation.value(),
      .paperSize = S::exportPdfPaperSize.value(),
      .addMetaData = S::exportPdfAddMetaData.value(),
      .metaDataTitle = S::exportPdfMetaDataTitle.value(),
      .metaDataAuthor = S::exportPdfAddMetaDataAuthor.value() ? S::exportPdfMetaDataAuthor.value() : "",
      .metaDataSubject = S::exportPdfAddMetaDataSubject.value() ? S::exportPdfMetaDataSubject.value() : "",
      .metaDataKeywords = S::exportPdfAddMetaDataKeywords.value() ? S::exportPdfMetaDataKeywords.value() : "",
    });
  }
};

struct Export3mfOptions {
  std::string colorMode = S::export3mfColorMode.defaultValue();
  std::string unit = S::export3mfUnit.defaultValue();
  std::string color = S::export3mfColor.defaultValue();
  std::string materialType = S::export3mfMaterialType.defaultValue();
  int decimalPrecision = S::export3mfDecimalPrecision.defaultValue();
  bool addMetaData = S::export3mfAddMetaData.defaultValue();
  std::string metaDataTitle;
  std::string metaDataDesigner;
  std::string metaDataDescription;
  std::string metaDataCopyright;
  std::string metaDataLicenseTerms;
  std::string metaDataRating;

  static const std::shared_ptr<const Export3mfOptions> fromSettings() {
    return std::make_shared<const Export3mfOptions>(Export3mfOptions{
      .colorMode = S::export3mfColorMode.value(),
      .unit = S::export3mfUnit.value(),
      .color = S::export3mfColor.value(),
      .materialType = S::export3mfMaterialType.value(),
      .decimalPrecision = S::export3mfDecimalPrecision.value(),
      .addMetaData = S::export3mfAddMetaData.value(),
      .metaDataTitle = S::export3mfMetaDataTitle.value(),
      .metaDataDesigner = S::export3mfAddMetaDataDesigner.value() ? S::export3mfMetaDataDesigner.value() : "",
      .metaDataDescription = S::export3mfAddMetaDataDescription.value() ? S::export3mfMetaDataDescription.value() : "",
      .metaDataCopyright = S::export3mfAddMetaDataCopyright.value() ? S::export3mfMetaDataCopyright.value() : "",
      .metaDataLicenseTerms = S::export3mfAddMetaDataLicenseTerms.value() ? S::export3mfMetaDataLicenseTerms.value() : "",
      .metaDataRating = S::export3mfAddMetaDataRating.value() ? S::export3mfMetaDataRating.value() : "",
    });
  }
};

struct ExportInfo {
  FileFormat format;
  FileFormatInfo info;
  std::string title;
  std::string sourceFilePath; // Full path to the OpenSCAD source file
  const Camera *camera;
  const Color4f defaultColor;
  const ColorScheme *colorScheme;

  std::shared_ptr<const ExportPdfOptions> optionsPdf;
  std::shared_ptr<const Export3mfOptions> options3mf;
};

ExportInfo createExportInfo(const FileFormat& format, const FileFormatInfo& info, const std::string& filepath, const Camera *camera);

bool exportFileByName(const std::shared_ptr<const class Geometry>& root_geom, const std::string& filename, const ExportInfo& exportInfo);
bool exportFileStdOut(const std::shared_ptr<const class Geometry>& root_geom, const ExportInfo& exportInfo);

void export_stl(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                bool binary = true);
void export_3mf(const std::shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo);
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

std::string get_current_iso8601_date_time_utc();

std::unique_ptr<OffscreenView> prepare_preview(Tree& tree, const ViewOptions& options, Camera& camera);
bool export_png(const std::shared_ptr<const class Geometry>& root_geom, const ViewOptions& options, Camera& camera, std::ostream& output);
bool export_png(const OffscreenView& glview, std::ostream& output);
bool export_param(SourceFile *root, const fs::path& path, std::ostream& output);

std::unique_ptr<PolySet> createSortedPolySet(const PolySet& ps);
