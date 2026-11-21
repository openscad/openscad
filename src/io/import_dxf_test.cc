#include <catch2/catch_all.hpp>
#include <cmath>
#include <filesystem>
#include <string>

#include "core/CurveDiscretizer.h"
#include "geometry/Polygon2d.h"
#include "io/DxfData.h"

namespace fs = std::filesystem;

namespace {

fs::path get_dxf_file_path(const std::string& name)
{
  return fs::path(OPENSCAD_TEST_DATA_DIR) / "dxf" / name;
}

}  // namespace

TEST_CASE("DXF import ignores lines with non-finite coordinates", "[io][DXF-Import]")
{
  const DxfData dd(CurveDiscretizer(0.0), get_dxf_file_path("non-finite.dxf").string());
  for (const auto& p : dd.points) {
    CHECK(std::isfinite(p.x()));
    CHECK(std::isfinite(p.y()));
  }

  // Only the triangle is left.
  const auto poly = dd.toPolygon2d();
  REQUIRE(poly);
  REQUIRE(poly->outlines().size() == 1);
  CHECK(poly->outlines()[0].vertices.size() == 3);
}
