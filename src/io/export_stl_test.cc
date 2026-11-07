#include "io/export.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>

#include "geometry/PolySet.h"
#include "platform/PlatformUtils.h"

#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

namespace {

/*! Unit cube as a triangular PolySet: 8 vertices, 12 triangles (outward winding). */
std::shared_ptr<PolySet> makeUnitCube()
{
  auto ps = std::make_shared<PolySet>(3);
  ps->vertices = {
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1},
  };
  ps->indices = {
    // bottom (-z)
    {0, 3, 2},
    {0, 2, 1},
    // top (+z)
    {4, 5, 6},
    {4, 6, 7},
    // front (-y)
    {0, 1, 5},
    {0, 5, 4},
    // back (+y)
    {2, 3, 7},
    {2, 7, 6},
    // right (+x)
    {1, 2, 6},
    {1, 6, 5},
    // left (-x)
    {0, 4, 7},
    {0, 7, 3},
  };
  ps->setTriangular(true);
  return ps;
}

uint32_t binaryStlTriangleCount(const std::string& s)
{
  const auto byte = [&s](size_t i) { return static_cast<uint32_t>(static_cast<unsigned char>(s[i])); };
  return byte(80) | (byte(81) << 8) | (byte(82) << 16) | (byte(83) << 24);
}

}  // namespace

TEST_CASE("STL export writes a valid binary header and triangle count", "[io][STL-Export]")
{
  std::ostringstream out;
  export_stl(makeUnitCube(), out, true, ExportStlOptions{});

  const auto s = out.str();
  REQUIRE(s.size() == 84 + 12 * 50);
  CHECK(s.compare(0, 15, "OpenSCAD Model\n") == 0);
  CHECK(std::all_of(s.begin() + 15, s.begin() + 80, [](char c) { return c == 0; }));
  CHECK(binaryStlTriangleCount(s) == 12);
}

TEST_CASE("STL export writes a valid ASCII file", "[io][STL-Export]")
{
  std::ostringstream out;
  export_stl(makeUnitCube(), out, false, ExportStlOptions{});

  const auto s = out.str();
  CHECK(s.rfind("solid OpenSCAD_Model\n", 0) == 0);
  const std::string endsolid = "endsolid OpenSCAD_Model\n";
  REQUIRE(s.size() > endsolid.size());
  CHECK(s.compare(s.size() - endsolid.size(), endsolid.size(), endsolid) == 0);

  size_t facets = 0;
  size_t pos = 0;
  while ((pos = s.find("facet normal", pos)) != std::string::npos) {
    ++facets;
    pos += 12;
  }
  CHECK(facets == 12);
}

#ifdef ENABLE_MANIFOLD
TEST_CASE("STL export applies optional manifold simplification", "[io][STL-Export][manifold]")
{
  // Manifold->PolySet conversion consults the ColorMap singleton, which requires
  // an initialised application path (the full binary registers one in main).
  PlatformUtils::registerApplicationPath(std::filesystem::current_path().string());

  auto geom = std::make_shared<ManifoldGeometry>(manifold::Manifold::Cylinder(1.0, 1.0, 1.0, 64));

  std::ostringstream outPlain;
  export_stl(geom, outPlain, true, ExportStlOptions{});
  const auto plainCount = binaryStlTriangleCount(outPlain.str());
  REQUIRE(plainCount > 0);

  ExportStlOptions options;
  options.simplifyTolerance = 0.5;
  std::ostringstream outSimplified;
  export_stl(geom, outSimplified, true, options);
  const auto simplifiedCount = binaryStlTriangleCount(outSimplified.str());

  CHECK(simplifiedCount <= plainCount);
}
#endif
