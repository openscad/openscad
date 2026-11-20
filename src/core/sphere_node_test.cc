#include "core/primitives.h"

#include <catch2/catch_all.hpp>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "core/CurveDiscretizer.h"
#include "geometry/Geometry.h"
#include "geometry/PolySet.h"
#include "geometry/linalg.h"

namespace {

std::unique_ptr<const Geometry> make_sphere(double r, double fn, const std::string& style)
{
  SphereNode node(nullptr, CurveDiscretizer(fn));
  node.r = r;
  node.style = style;
  return node.createGeometry();
}

std::unique_ptr<const Geometry> make_cylinder(double r, double fn)
{
  CylinderNode node(nullptr, CurveDiscretizer(fn));
  node.r1 = node.r2 = r;
  node.h = 1;
  node.center = true;
  return node.createGeometry();
}

using Vertex = std::tuple<double, double, double>;

Vertex rounded(const Vector3d& v)
{
  return {std::round(v.x() * 1e9), std::round(v.y() * 1e9), std::round(v.z() * 1e9)};
}

}  // namespace

TEST_CASE("SphereNode style is dumped", "[core][SphereNode]")
{
  SphereNode node(nullptr, CurveDiscretizer(8));
  CHECK_THAT(node.toString(), Catch::Matchers::ContainsSubstring("style = \"orig\""));
  node.style = "octa";
  CHECK_THAT(node.toString(), Catch::Matchers::ContainsSubstring("style = \"octa\""));
}

TEST_CASE("SphereNode octa style mesh", "[core][SphereNode]")
{
  const double r = 2.5;
  const int fn = GENERATE(5, 8, 12, 20, 100);
  const int n = (fn + 3) / 4;  // subdivisions per octahedron edge
  INFO("fn = " << fn);

  const auto geom = make_sphere(r, fn, "octa");
  const auto *ps = dynamic_cast<const PolySet *>(geom.get());
  REQUIRE(ps != nullptr);

  SECTION("is a closed, consistently oriented triangle mesh on the sphere")
  {
    REQUIRE(ps->vertices.size() == 4 * n * n + 2);
    REQUIRE(ps->indices.size() == 8 * n * n);

    for (const auto& v : ps->vertices) {
      CHECK(v.norm() == Catch::Approx(r).epsilon(1e-12));
    }

    std::map<std::pair<int, int>, int> edges;
    for (const auto& face : ps->indices) {
      REQUIRE(face.size() == 3);
      const Vector3d& a = ps->vertices[face[0]];
      const Vector3d& b = ps->vertices[face[1]];
      const Vector3d& c = ps->vertices[face[2]];
      // Counterclockwise as seen from outside
      CHECK((b - a).cross(c - a).dot(a + b + c) > 0);
      for (int i = 0; i < 3; ++i) {
        CHECK(face[i] != face[(i + 1) % 3]);
        edges[{face[i], face[(i + 1) % 3]}]++;
      }
    }
    for (const auto& [edge, count] : edges) {
      CHECK(count == 1);
      CHECK(edges[{edge.second, edge.first}] == 1);
    }
  }

  SECTION("has the poles and the symmetry of the octahedron")
  {
    std::set<Vertex> vertices;
    for (const auto& v : ps->vertices) vertices.insert(rounded(v));
    for (const auto& pole : {Vector3d(r, 0, 0), Vector3d(-r, 0, 0), Vector3d(0, r, 0),
                             Vector3d(0, -r, 0), Vector3d(0, 0, r), Vector3d(0, 0, -r)}) {
      CHECK(vertices.count(rounded(pole)) == 1);
    }
    for (const auto& v : ps->vertices) {
      CHECK(vertices.count(rounded(Vector3d(v.y(), v.z(), v.x()))) == 1);
      CHECK(vertices.count(rounded(Vector3d(v.y(), v.x(), v.z()))) == 1);
      CHECK(vertices.count(rounded(Vector3d(-v.x(), v.y(), v.z()))) == 1);
      CHECK(vertices.count(rounded(Vector3d(v.x(), -v.y(), v.z()))) == 1);
      CHECK(vertices.count(rounded(Vector3d(v.x(), v.y(), -v.z()))) == 1);
    }
  }

  SECTION("shares the vertices of a cylinder with the same fragments on all three axes")
  {
    const auto cylinder = make_cylinder(r, 4 * n);
    const auto *cps = dynamic_cast<const PolySet *>(cylinder.get());
    REQUIRE(cps != nullptr);
    std::set<std::pair<double, double>> circle;
    for (const auto& v : cps->vertices) {
      if (v.z() > 0) circle.emplace(v.x(), v.y());
    }
    REQUIRE(circle.size() == 4 * n);

    std::set<std::pair<double, double>> equator, xz_meridian, yz_meridian;
    for (const auto& v : ps->vertices) {
      if (v.z() == 0) equator.emplace(v.x(), v.y());
      if (v.y() == 0) xz_meridian.emplace(v.x(), v.z());
      if (v.x() == 0) yz_meridian.emplace(v.y(), v.z());
    }
    CHECK(equator == circle);
    CHECK(xz_meridian == circle);
    CHECK(yz_meridian == circle);
  }
}

TEST_CASE("SphereNode orig style mesh", "[core][SphereNode]")
{
  const auto geom = make_sphere(1, 10, "orig");
  const auto *ps = dynamic_cast<const PolySet *>(geom.get());
  REQUIRE(ps != nullptr);
  CHECK(ps->vertices.size() == 5 * 10);
  CHECK(ps->indices.size() == 4 * 10 + 2);
}
