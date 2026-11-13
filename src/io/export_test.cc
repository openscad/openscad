#include "io/export.h"

#include <catch2/catch_all.hpp>
#include <locale>
#include <memory>
#include <sstream>
#include <string>

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
