#include <catch2/catch_all.hpp>
#include "linear_extrude.h"

#include <algorithm>
#include <iterator>
#include <cassert>
#include <cmath>
#include <utility>
#include <memory>
#include <cstddef>
#include <vector>

#include <boost/logic/tribool.hpp>

#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/GeometryUtils.h"
#include "glview/RenderSettings.h"
#include "core/LinearExtrudeNode.h"
#include "geometry/manifold/manifoldutils.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "utils/degree_trig.h"

namespace LinearExtrudeInternals {
std::unique_ptr<PolySet> assemblePolySetForManifold(const Polygon2d& polyref,
                                                    std::vector<Vector3d>&& vertices,
                                                    PolygonIndices&& indices, int convexity,
                                                    boost::tribool isConvex, int index_offset,
                                                    bool use_alternative_copy = false);

void prepareVerticesAndIndices(const Polygon2d& polyref, Vector3d h1, Vector3d h2, int num_slices,
                               double scale_x, double scale_y, double twist,
                               std::vector<Vector3d>& vertices, PolygonIndices& indices,
                               int& slice_stride);
}  // namespace LinearExtrudeInternals

Polygon2d makeSquare(double s)
{
  Vector2d v1(0, 0);
  Vector2d v2(s, s);

  Outline2d o;
  o.vertices = {v1, {v2[0], v1[1]}, v2, {v1[0], v2[1]}};
  return std::move(Polygon2d(o));
}

Polygon2d makeCross(double s)
{
  Outline2d o;
  o.vertices = {{0, 0}, {s / 2, s / 4},     {s, 0}, {s * 3 / 4, s / 2},
                {s, s}, {s / 2, s * 3 / 4}, {0, s}, {s / 4, s / 2}};
  return std::move(Polygon2d(o));
}

TEST_CASE("assemblePolySetForManifold passes some basic checks")
{
  auto polyref = makeCross(3.0);

  Vector3d h1 = Vector3d::Zero();
  Vector3d h2 = Vector3d(0, 0, 5.0);
  double twist = 0;

  int num_slices = 2;
  int slice_stride = 0;
  std::vector<Vector3d> vertices;
  PolygonIndices indices;
  LinearExtrudeInternals::prepareVerticesAndIndices(polyref, h1, h2, num_slices, 1.0, 1.0, twist,
                                                    vertices, indices, slice_stride);

  SECTION("both copy algorithms make the same shape")
  {
    std::vector<Vector3d> vertices_copy = vertices;
    PolygonIndices indices_copy = indices;
    auto ps1 = LinearExtrudeInternals::assemblePolySetForManifold(
      polyref, std::move(vertices), std::move(indices), 1, unknown, slice_stride * num_slices, false);
    auto ps2 = LinearExtrudeInternals::assemblePolySetForManifold(polyref, std::move(vertices_copy),
                                                                  std::move(indices_copy), 1, unknown,
                                                                  slice_stride * num_slices, true);
    auto geom1 = ManifoldUtils::createManifoldFromPolySet(*ps1)->getManifold();
    auto geom2 = ManifoldUtils::createManifoldFromPolySet(*ps2)->getManifold();
    CHECK((geom1 - geom2).Volume() == 0);
    CHECK((geom2 - geom1).Volume() == 0);
    CHECK((geom1 ^ geom1).Volume() == geom1.Volume());
    CHECK(geom1.Volume() == geom2.Volume());
  }
}
