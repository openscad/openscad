#include "io/import.h"

#include <boost/none.hpp>
#include <catch2/catch_all.hpp>
#include <string>

#include "core/AST.h"
#include "core/CurveDiscretizer.h"
#include "geometry/Polygon2d.h"

namespace {

std::string get_svg_file_path(const std::string& name)
{
  return std::string(OPENSCAD_TEST_DATA_DIR) + "/svg/" + name;
}

const Location test_location{0, 0, 0, 0, nullptr};

}  // namespace

// viewbox-offset.svg: viewBox="100 50 30 20" on a 60mm x 40mm page, so one
// user unit is 2mm. The path is a 10 x 5 rectangle at (110, 55), i.e.
// 20mm x 10mm.

TEST_CASE("SVG import with center = true ignores the viewBox origin", "[io][SVG-Import]")
{
  const auto poly = import_svg(CurveDiscretizer(0.0), get_svg_file_path("viewbox-offset.svg"),
                               boost::none, boost::none, 72.0, true, test_location);
  REQUIRE(poly);
  REQUIRE_FALSE(poly->isEmpty());

  using Catch::Matchers::WithinAbs;
  const auto bbox = poly->getBoundingBox();
  CHECK_THAT(bbox.min().x(), WithinAbs(-10.0, 1e-6));
  CHECK_THAT(bbox.max().x(), WithinAbs(10.0, 1e-6));
  CHECK_THAT(bbox.min().y(), WithinAbs(-5.0, 1e-6));
  CHECK_THAT(bbox.max().y(), WithinAbs(5.0, 1e-6));
}
