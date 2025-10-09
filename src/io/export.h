#pragma once

#include <filesystem>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

#include "core/Settings.h"
#include "core/Tree.h"
#include "core/SourceFile.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "glview/Camera.h"
#include "glview/ColorMap.h"
#include "io/export_enums.h"

using SPDF = Settings::SettingsExportPdf;
using S3MF = Settings::SettingsExport3mf;

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

}  // namespace fileformat

using CmdLineExportOptions =
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

template <typename settings_entry_type>
auto set_cmd_line_option(const CmdLineExportOptions& cmdLineOptions, const std::string& section,
                         const settings_entry_type& se)
{
  if (cmdLineOptions.count(section) == 0) {
    return se.defaultValue();
  }

  const auto& o = cmdLineOptions.at(section);
  if (o.count(se.name()) == 0) {
    return se.defaultValue();
  }

  return se.decode(o.at(se.name()));
}

// include defaults to use without dialog or direction.
// Defaults match values used prior to incorporation of options.
struct ExportPdfOptions {
  bool showScale = true;
  bool showScaleMsg = true;
  bool showGrid = false;
  double gridSize = 10.0;
  bool showDesignFilename = false;
  ExportPdfPaperOrientation orientation = ExportPdfPaperOrientation::PORTRAIT;
  ExportPdfPaperSize paperSize = ExportPdfPaperSize::A4;
  bool addMetaData = SPDF::exportPdfAddMetaData.defaultValue();
  std::string metaDataTitle;
  std::string metaDataAuthor;
  std::string metaDataSubject;
  std::string metaDataKeywords;
  bool fill = false;
  std::string fillColor = "black";
  bool stroke = true;
  std::string strokeColor = "black";
  double strokeWidth = 1;

  static std::shared_ptr<const ExportPdfOptions> withOptions(const CmdLineExportOptions& cmdLineOptions)
  {
    return std::make_shared<const ExportPdfOptions>(ExportPdfOptions{
      .showScale = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                       Settings::SettingsExportPdf::exportPdfShowScale),
      .showScaleMsg = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                          Settings::SettingsExportPdf::exportPdfShowScaleMessage),
      .showGrid = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                      Settings::SettingsExportPdf::exportPdfShowGrid),
      .gridSize = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                      Settings::SettingsExportPdf::exportPdfGridSize),
      .showDesignFilename = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                                Settings::SettingsExportPdf::exportPdfShowFilename),
      .orientation = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                         Settings::SettingsExportPdf::exportPdfOrientation),
      .paperSize = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                       Settings::SettingsExportPdf::exportPdfPaperSize),
      .addMetaData = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                         Settings::SettingsExportPdf::exportPdfAddMetaData),
      .metaDataTitle = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                           Settings::SettingsExportPdf::exportPdfMetaDataTitle),
      .metaDataAuthor = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                            Settings::SettingsExportPdf::exportPdfMetaDataAuthor),
      .metaDataSubject = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                             Settings::SettingsExportPdf::exportPdfMetaDataSubject),
      .metaDataKeywords = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                              Settings::SettingsExportPdf::exportPdfMetaDataKeywords),
      .fill = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                  Settings::SettingsExportPdf::exportPdfFill),
      .fillColor = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                       Settings::SettingsExportPdf::exportPdfFillColor),
      .stroke = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                    Settings::SettingsExportPdf::exportPdfStroke),
      .strokeColor = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                         Settings::SettingsExportPdf::exportPdfStrokeColor),
      .strokeWidth = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_PDF,
                                         Settings::SettingsExportPdf::exportPdfStrokeWidth),
    });
  }

  static const std::shared_ptr<const ExportPdfOptions> fromSettings()
  {
    return std::make_shared<const ExportPdfOptions>(ExportPdfOptions{
      .showScale = SPDF::exportPdfShowScale.value(),
      .showScaleMsg = SPDF::exportPdfShowScaleMessage.value(),
      .showGrid = SPDF::exportPdfShowGrid.value(),
      .gridSize = SPDF::exportPdfGridSize.value(),
      .showDesignFilename = SPDF::exportPdfShowFilename.value(),
      .orientation = SPDF::exportPdfOrientation.value(),
      .paperSize = SPDF::exportPdfPaperSize.value(),
      .addMetaData = SPDF::exportPdfAddMetaData.value(),
      .metaDataTitle = SPDF::exportPdfMetaDataTitle.value(),
      .metaDataAuthor =
        SPDF::exportPdfAddMetaDataAuthor.value() ? SPDF::exportPdfMetaDataAuthor.value() : "",
      .metaDataSubject =
        SPDF::exportPdfAddMetaDataSubject.value() ? SPDF::exportPdfMetaDataSubject.value() : "",
      .metaDataKeywords =
        SPDF::exportPdfAddMetaDataKeywords.value() ? SPDF::exportPdfMetaDataKeywords.value() : "",
      .fill = SPDF::exportPdfFill.value(),
      .fillColor = SPDF::exportPdfFillColor.value(),
      .stroke = SPDF::exportPdfStroke.value(),
      .strokeColor = SPDF::exportPdfStrokeColor.value(),
      .strokeWidth = SPDF::exportPdfStrokeWidth.value(),
    });
  }
};

struct Export3mfOptions {
  Export3mfColorMode colorMode;
  Export3mfUnit unit;
  std::string color;
  Export3mfMaterialType materialType;
  int decimalPrecision;
  bool addMetaData;
  std::string metaDataTitle;
  std::string metaDataDesigner;
  std::string metaDataDescription;
  std::string metaDataCopyright;
  std::string metaDataLicenseTerms;
  std::string metaDataRating;

  static const std::shared_ptr<const Export3mfOptions> withOptions(
    const CmdLineExportOptions& cmdLineOptions)
  {
    return std::make_shared<const Export3mfOptions>(Export3mfOptions{
      .colorMode = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                       Settings::SettingsExport3mf::export3mfColorMode),
      .unit = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                  Settings::SettingsExport3mf::export3mfUnit),
      .color = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                   Settings::SettingsExport3mf::export3mfColor),
      .materialType = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                          Settings::SettingsExport3mf::export3mfMaterialType),
      .decimalPrecision = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                              Settings::SettingsExport3mf::export3mfDecimalPrecision),
      .addMetaData = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                         Settings::SettingsExport3mf::export3mfAddMetaData),
      .metaDataTitle = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                           Settings::SettingsExport3mf::export3mfMetaDataTitle),
      .metaDataDesigner = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                              Settings::SettingsExport3mf::export3mfMetaDataDesigner),
      .metaDataDescription =
        set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                            Settings::SettingsExport3mf::export3mfMetaDataDescription),
      .metaDataCopyright = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                               Settings::SettingsExport3mf::export3mfMetaDataCopyright),
      .metaDataLicenseTerms =
        set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                            Settings::SettingsExport3mf::export3mfMetaDataLicenseTerms),
      .metaDataRating = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_3MF,
                                            Settings::SettingsExport3mf::export3mfMetaDataRating),
    });
  }

  static const std::shared_ptr<const Export3mfOptions> fromSettings()
  {
    return std::make_shared<const Export3mfOptions>(Export3mfOptions{
      .colorMode = S3MF::export3mfColorMode.value(),
      .unit = S3MF::export3mfUnit.value(),
      .color = S3MF::export3mfColor.value(),
      .materialType = S3MF::export3mfMaterialType.value(),
      .decimalPrecision = S3MF::export3mfDecimalPrecision.value(),
      .addMetaData = S3MF::export3mfAddMetaData.value(),
      .metaDataTitle = S3MF::export3mfMetaDataTitle.value(),
      .metaDataDesigner =
        S3MF::export3mfAddMetaDataDesigner.value() ? S3MF::export3mfMetaDataDesigner.value() : "",
      .metaDataDescription =
        S3MF::export3mfAddMetaDataDescription.value() ? S3MF::export3mfMetaDataDescription.value() : "",
      .metaDataCopyright =
        S3MF::export3mfAddMetaDataCopyright.value() ? S3MF::export3mfMetaDataCopyright.value() : "",
      .metaDataLicenseTerms = S3MF::export3mfAddMetaDataLicenseTerms.value()
                                ? S3MF::export3mfMetaDataLicenseTerms.value()
                                : "",
      .metaDataRating =
        S3MF::export3mfAddMetaDataRating.value() ? S3MF::export3mfMetaDataRating.value() : "",
    });
  }
};

struct ExportSvgOptions {
  bool fill = false;
  std::string fillColor = "white";
  bool stroke = true;
  std::string strokeColor = "black";
  double strokeWidth = 0.35;

  static std::shared_ptr<const ExportSvgOptions> withOptions(const CmdLineExportOptions& cmdLineOptions)
  {
    return std::make_shared<const ExportSvgOptions>(ExportSvgOptions{
      .fill = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_SVG,
                                  Settings::SettingsExportSvg::exportSvgFill),
      .fillColor = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_SVG,
                                       Settings::SettingsExportSvg::exportSvgFillColor),
      .stroke = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_SVG,
                                    Settings::SettingsExportSvg::exportSvgStroke),
      .strokeColor = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_SVG,
                                         Settings::SettingsExportSvg::exportSvgStrokeColor),
      .strokeWidth = set_cmd_line_option(cmdLineOptions, Settings::SECTION_EXPORT_SVG,
                                         Settings::SettingsExportSvg::exportSvgStrokeWidth),
    });
  }

  static const std::shared_ptr<const ExportSvgOptions> fromSettings()
  {
    return std::make_shared<const ExportSvgOptions>(ExportSvgOptions{
      .fill = Settings::SettingsExportSvg::exportSvgFill.value(),
      .fillColor = Settings::SettingsExportSvg::exportSvgFillColor.value(),
      .stroke = Settings::SettingsExportSvg::exportSvgStroke.value(),
      .strokeColor = Settings::SettingsExportSvg::exportSvgStrokeColor.value(),
      .strokeWidth = Settings::SettingsExportSvg::exportSvgStrokeWidth.value(),
    });
  }
};

struct ExportInfo {
  FileFormat format;
  FileFormatInfo info;
  std::string title;
  std::string sourceFilePath;  // Full path to the OpenSCAD source file
  const Camera *camera;
  const Color4f defaultColor;
  const ColorScheme *colorScheme;

  std::shared_ptr<const ExportPdfOptions> optionsPdf;
  std::shared_ptr<const Export3mfOptions> options3mf;
  std::shared_ptr<const ExportSvgOptions> optionsSvg;
};

ExportInfo createExportInfo(const FileFormat& format, const FileFormatInfo& info,
                            const std::string& filepath, const Camera *camera,
                            const CmdLineExportOptions& cmdLineOptions);

bool exportFileByName(const std::shared_ptr<const class Geometry>& root_geom,
                      const std::string& filename, const ExportInfo& exportInfo);
bool exportFileStdOut(const std::shared_ptr<const class Geometry>& root_geom,
                      const ExportInfo& exportInfo);

void export_stl(const std::shared_ptr<const Geometry>& geom, std::ostream& output, bool binary = true);
void export_3mf(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                const ExportInfo& exportInfo);
void export_obj(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_off(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_wrl(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_amf(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_svg(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                const ExportInfo& exportInfo);
void export_pov(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                const ExportInfo& exportInfo);
void export_pdf(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                const ExportInfo& exportInfo);
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
    {"crosshairs", false},
  };

  const std::vector<std::string> names()
  {
    std::vector<std::string> names;
    boost::copy(flags | boost::adaptors::map_keys, std::back_inserter(names));
    return names;
  }

  bool& operator[](const std::string& name) { return flags.at(name); }

  bool operator[](const std::string& name) const { return flags.at(name); }
};

class OffscreenView;

std::string get_current_iso8601_date_time_utc();

std::unique_ptr<OffscreenView> prepare_preview(Tree& tree, const ViewOptions& options, Camera& camera);
bool export_png(const std::shared_ptr<const class Geometry>& root_geom, const ViewOptions& options,
                Camera& camera, std::ostream& output);
bool export_png(const OffscreenView& glview, std::ostream& output);
bool export_param(SourceFile *root, const fs::path& path, std::ostream& output);

std::unique_ptr<PolySet> createSortedPolySet(const PolySet& ps);
