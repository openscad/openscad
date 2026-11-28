#include "io/export.h"

#include <catch2/catch_all.hpp>
#include <filesystem>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "geometry/PolySet.h"
#include "geometry/Polygon2d.h"
#include "utils/scope_guard.hpp"

namespace {

// A decimal comma, as used by the number format of many system locales.
struct DecimalComma : std::numpunct<char> {
  char do_decimal_point() const override { return ','; }
};

std::string export_to_string(const std::shared_ptr<const Geometry>& geom, FileFormat format)
{
  std::ostringstream output;  // starts with the global locale, which exportFile() must override
  exportFile(geom, output, ExportInfo{.format = format, .info = fileformat::info(format)});
  return output.str();
}

}  // namespace

TEST_CASE("SVG export escapes the colors from the export options", "[io][export]")
{
  Outline2d outline;
  outline.vertices = {{0, 0}, {1, 0}, {0, 1}};
  const auto shape = std::make_shared<const Polygon2d>(outline);

  ExportSvgOptions options;
  options.fill = true;
  options.fillColor = R"(red" onload="x)";
  options.strokeColor = "<&'>";
  std::ostringstream output;
  exportFile(shape, output,
             ExportInfo{.format = FileFormat::SVG,
                        .info = fileformat::info(FileFormat::SVG),
                        .optionsSvg = std::make_shared<const ExportSvgOptions>(options)});
  const std::string svg = output.str();
  CHECK(svg.find(R"(fill="red&quot; onload=&quot;x")") != std::string::npos);
  CHECK(svg.find(R"(stroke="&lt;&amp;&apos;&gt;")") != std::string::npos);
  CHECK(svg.find("onload=\"") == std::string::npos);
}

TEST_CASE("Exports use '.' as the decimal point whatever the global C++ locale is", "[io][export]")
{
  const std::locale comma_locale(std::locale::classic(), new DecimalComma);
  const std::locale previous = std::locale::global(comma_locale);
  auto restore = sg::make_scope_guard([&]() { std::locale::global(previous); });

  Outline2d outline;
  outline.vertices = {{0.25, 0.25}, {2.25, 0.25}, {1.25, 1.75}};
  const auto shape = std::make_shared<const Polygon2d>(outline);

  for (const auto format : {FileFormat::DXF, FileFormat::SVG}) {
    INFO(fileformat::info(format).identifier);
    const std::string output = export_to_string(shape, format);
    CHECK(output.find("0.25") != std::string::npos);
    CHECK(output.find("0,25") == std::string::npos);
  }
}

TEST_CASE("Write errors during export are reported instead of aborting", "[io][export]")
{
  // /dev/full accepts the open but fails every write with ENOSPC.
  if (!std::filesystem::exists("/dev/full")) SKIP("no /dev/full on this system");

  Outline2d outline;
  outline.vertices = {{0, 0}, {1, 0}, {0, 1}};
  const auto shape = std::make_shared<const Polygon2d>(outline);

  std::vector<FileFormat> formats = {FileFormat::DXF, FileFormat::SVG};
#ifdef ENABLE_CAIRO
  formats.push_back(FileFormat::PDF);
#endif
  for (const auto format : formats) {
    INFO(fileformat::info(format).identifier);
    const ExportInfo info{.format = format, .info = fileformat::info(format)};
    CHECK_FALSE(exportFileByName(shape, "/dev/full", info));
  }

  auto tetrahedron = std::make_shared<PolySet>(3);
  tetrahedron->vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  tetrahedron->indices = {{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {0, 3, 2}};
  std::vector<FileFormat> formats3d = {FileFormat::OFF, FileFormat::ASCII_STL, FileFormat::BINARY_STL};
#ifdef ENABLE_LIB3MF
  formats3d.push_back(FileFormat::_3MF);
#endif
  for (const auto format : formats3d) {
    INFO(fileformat::info(format).identifier);
    const ExportInfo info{.format = format, .info = fileformat::info(format)};
    CHECK_FALSE(exportFileByName(tetrahedron, "/dev/full", info));
  }
}
