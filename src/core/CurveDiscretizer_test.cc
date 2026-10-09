#include "core/CurveDiscretizer.h"

#include <catch2/catch_all.hpp>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include "geometry/Polygon2d.h"

namespace {
CurveDiscretizer makeDiscretizerWithFn(double fn)
{
  return CurveDiscretizer([fn](const char *name) -> std::optional<double> {
    if (std::string(name) == "fn") return fn;
    return std::nullopt;
  });
}
}  // namespace

TEST_CASE("splitOutline reaches the requested $fn even when edges tie on length",
          "[CurveDiscretizer][splitOutline]")
{
  // A regular polygon has every edge the same length, so every edge ties on the
  // "which edge to subdivide next" metric used by splitOutlineByFn. The outline must
  // still end up with at least as many vertices as $fn requests.
  Outline2d square;
  square.vertices = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
  square.positive = true;

  Outline2d triangle;
  triangle.vertices = {{0, 0}, {1, 0}, {0.5, 0.8660254}};
  triangle.positive = true;

  struct TestCase {
    std::string name;
    Outline2d outline;
    double fn;
  };
  const std::vector<TestCase> cases = {
    {"square, fn=10", square, 10.0},
    {"square, fn=20", square, 20.0},
    {"triangle, fn=10", triangle, 10.0},
  };

  for (const auto& test : cases) {
    SECTION(test.name)
    {
      CurveDiscretizer discretizer = makeDiscretizerWithFn(test.fn);
      Outline2d result =
        discretizer.splitOutline(test.outline, /*twist=*/0.0, /*scale_x=*/1.0, /*scale_y=*/1.0,
                                 /*slices=*/1, /*segments=*/0);
      CHECK(result.vertices.size() >= static_cast<size_t>(test.fn));
    }
  }
}

TEST_CASE("splitOutline spreads a partial split evenly instead of clustering it",
          "[CurveDiscretizer][splitOutline]")
{
  // Regular hexagon: 6 equal edges all tie on the subdivision metric. With $fn=9, only
  // 3 of the 6 can be split once (budget=3), so the result should split every other
  // edge (0, 2, 4) rather than three adjacent ones, keeping the hexagon's symmetry.
  // That means the resulting 9 edge lengths alternate: two half-length edges from each
  // split original edge, then one full-length edge from each unsplit original edge.
  Outline2d hexagon;
  for (int i = 0; i < 6; ++i) {
    double angle = i * M_PI / 3.0;
    hexagon.vertices.emplace_back(cos(angle), sin(angle));
  }
  hexagon.positive = true;

  CurveDiscretizer discretizer = makeDiscretizerWithFn(9.0);
  Outline2d result = discretizer.splitOutline(hexagon, 0.0, 1.0, 1.0, 1, 0);
  REQUIRE(result.vertices.size() == 9);

  const double full_edge = (hexagon.vertices[1] - hexagon.vertices[0]).norm();
  std::vector<double> edge_lengths;
  for (size_t i = 0; i < result.vertices.size(); ++i) {
    edge_lengths.push_back(
      (result.vertices[(i + 1) % result.vertices.size()] - result.vertices[i]).norm());
  }

  // Expected pattern around the hexagon: half, half, full, half, half, full, half, half, full.
  const std::vector<bool> is_half_length = {true, true, false, true, true, false, true, true, false};
  for (size_t i = 0; i < edge_lengths.size(); ++i) {
    INFO("edge " << i << " length=" << edge_lengths[i]);
    if (is_half_length[i]) {
      CHECK(edge_lengths[i] == Catch::Approx(full_edge / 2.0).margin(1e-9));
    } else {
      CHECK(edge_lengths[i] == Catch::Approx(full_edge).margin(1e-9));
    }
  }
}
