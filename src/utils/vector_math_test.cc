#include <catch2/catch_all.hpp>
#include "vector_math.h"

#define NOT_APPLICABLE 0.0

TEST_CASE("calculateLinePointDistance calculates distance to infinite line", "[Geometry][LinePoint]")
{
  struct LinePointTestData {
    std::string name;
    Vector3d l1b;
    Vector3d l1e;
    Vector3d pt;
    double expected_dist;
    double expected_lat;
  };

  const LinePointTestData testCases[] = {
    {"On Line (Midpoint)", {0, 0, 0}, {2, 0, 0}, {1, 0, 0}, 0.0, 1.0},

    {"Perpendicular at Start (t=0)", {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 1.0, 0.0},

    {"Perpendicular at End (t=1)", {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, 1.0, 1.0},

    {"Projection Outside Segment (t=2)", {0, 0, 0}, {1, 0, 0}, {2, 1, 0}, 1.41421356237309515, 1.0},

    {"Projection Negative (t=-1)", {0, 0, 0}, {1, 0, 0}, {-1, 1, 0}, 1.41421356237309515, 0},

    {"Function documentation", {1, 1, 1}, {-4, 1, 1}, {0, 0, 0}, 1.41421356237309515, 1.0},

    // Closest point is (2,0,0)
    {"3D Offset (t=2/3)", {0, 0, 0}, {3, 0, 0}, {2, 4, 3}, 5.0, 2.0},

    {"Line is a Point", {5, 5, 5}, {5, 5, 5}, {5, 6, 5}, 1.0, NOT_APPLICABLE}};

  const double epsilon = 1e-6;

  for (const auto& test : testCases) {
    SECTION(test.name)
    {
      double actual_lat;
      double actual_dist = calculateLinePointDistance(test.l1b, test.l1e, test.pt, actual_lat);

      CHECK(actual_dist == Catch::Approx(test.expected_dist).margin(epsilon));
      CHECK(actual_lat == Catch::Approx(test.expected_lat).margin(epsilon));
    }
  }
}

TEST_CASE("calculateSegSegDistance handles standard geometry", "[vector_math][segment_distance]")
{
  struct SegSegTestData {
    std::string name;
    Vector3d l1b, l1e, l2b, l2e;
    double expected_distance;
  };

  std::vector<SegSegTestData> test_cases = {
    {"Intersecting Segments on X/Y axis", Vector3d(0.0, 0.0, 0.0),
     Vector3d(2.0, 0.0, 0.0),                            // S1: X-axis segment
     Vector3d(1.0, -1.0, 0.0), Vector3d(1.0, 1.0, 0.0),  // S2: Y-axis segment (intersects S1 at 1,0,0)
     0.0},
    {"Parallel X-axis segments, Z=0, Y-sep=1", Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 0.0, 0.0),
     Vector3d(0.0, 1.0, 0.0), Vector3d(1.0, 1.0, 0.0), 1.0},
    {"Touching end-to-end (collinear)", Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 0.0, 0.0),
     Vector3d(1.0, 0.0, 0.0), Vector3d(2.0, 0.0, 0.0), 0.0},
    {
      "Skewed, X-axis vs Y-axis at Z=5", Vector3d(0.0, 0.0, 0.0),
      Vector3d(10.0, 0.0, 0.0),                           // S1: Along X-axis
      Vector3d(0.0, 5.0, 5.0), Vector3d(0.0, -5.0, 5.0),  // S2: Along Y-axis at Z=5
      5.0  // Shortest distance is between (0,0,0) on S1 and (0,0,5) on the line S2 is on.
    },
    {"Point-to-Point distance, 5 units apart", Vector3d(0.0, 0.0, 0.0), Vector3d(0.0, 0.0, 0.0),
     Vector3d(5.0, 0.0, 0.0), Vector3d(5.0, 0.0, 0.0), 5.0},
    // Following added for manually-detected bug:
    {"Collinear, where start of second segment is further than end of second segment",
     Vector3d(0.0, 0.0, 1.0), Vector3d(0.0, 0.0, 0.0), Vector3d(0.0, 0.0, 6.0), Vector3d(0.0, 0.0, 5.0),
     4.0},
    {"Displacement of unit line segments, Z+4, X+1 units apart", Vector3d(0.0, 0.0, 0.0),
     Vector3d(0.0, 0.0, 1.0), Vector3d(1.0, 0.0, 6.0), Vector3d(1.0, 0.0, 5.0), 4.12311},

  };

  for (const auto& test : test_cases) {
    SECTION(test.name)
    {
      double actual_distance = calculateSegSegDistance(test.l1b, test.l1e, test.l2b, test.l2e);

      INFO("S1: [" << test.l1b << " to " << test.l1e << "], S2: [" << test.l2b << " to " << test.l2e
                   << "]");
      INFO("Expected: " << test.expected_distance << ", Actual: " << actual_distance);

      REQUIRE(actual_distance == Catch::Approx(test.expected_distance).margin(1e-6));
    }
  }
}

const auto NaN = std::numeric_limits<double>::quiet_NaN();

TEST_CASE("calculateLineLineDistance handles various line arrangements (Eigen)", "[Geometry][Line]")
{
  struct LineLineTestData {
    std::string name;
    Vector3d l1b;
    Vector3d l1e;
    Vector3d l2b;
    Vector3d l2e;
    double expected_dist;  // The signed shortest distance (returned 'd')
    double expected_t;     // The `t` or `s` from the parametric line equations ('parametric_t')
  };

  const LineLineTestData testCases[] = {
    // L1: x-axis. L2: y-axis, shifted z=1. Shortest distance: 1.0 (along Z).
    {"Skew: Axis-aligned", {0, 0, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1, 1}, -1.0, 0.0},

    // L1: xz-plane. L2: yz-plane. Intersection at (0,0,0).
    {"Intersecting at Origin",
     {-1, 0, 0},
     {1, 0, 0},  // L1 (x-axis)
     {0, -1, 0},
     {0, 1, 0},  // L2 (y-axis)
     0.0,
     0.5},

    // L1: x-axis. L2: Parallel, shifted by 1 unit on the y-axis.
    {"Parallel", {0, 0, 0}, {5, 0, 0}, {0, 1, 0}, {10, 1, 0}, 1.0, NaN},

    // L1: x-axis. L2: Parallel, shifted by 1 unit on the y-axis.
    {"Parallel reversed second line", {0, 0, 0}, {1, 0, 0}, {10, 1, 0}, {0, 1, 0}, 1.0, NaN},

    // L1: x-axis (0,0,0) to (1,0,0). v1=(1,0,0)
    // L2: Parallel to z-axis, shifted by (1,1,0). (1,1,1) to (1,1,2). v2=(0,0,1)
    {"Complex Skew with Lateral Distance", {0, 0, 0}, {1, 0, 0}, {1, 1, 1}, {1, 1, 2}, 1.0, 1.0},

    // L1: x-axis. L2: Parallel to y-axis, shifted by z=10, x=2.
    {"Skew: A little apart",
     {0, 0, 0},
     {1, 0, 0},  // v1=(1,0,0)
     {2, 0, 10},
     {2, 1, 10},  // v2=(0,1,0)
     -10.0,
     2.0},

    // https://www.ambrbit.com/TrigoCalc/Line3D/Distance2Lines3D_.htm helped here.
    {"Skew: Further apart",
     {0, -200, -200},
     {300, -200, 200},  // v1=(300,0,400); r = (0, -200, -200) + t(300, 0, 400)
     {1000, 1000, -2000},
     {-800, 500, 100},  // v2=(-1800,-500,2100);
     837.6106,
     -0.77666},

    // L1: x-axis. L2: Parallel, shifted by 1 unit on the y-axis.
    {"Collinear, second line first endpoint further away",
     {0, 0, 0},
     {1, 0, 0},
     {3, 0, 0},
     {2, 0, 0},
     0.0,
     NaN},

    // L1: x-axis. L2: Parallel, shifted by 1 unit on the y-axis.
    {"Collinear, overlapping, second line first endpoint further away",
     {0, 0, 0},
     {1, 0, 0},
     {3, 0, 0},
     {0.5, 0, 0},
     0.0,
     NaN},

  };

  const double epsilon = 1e-4;

  for (const auto& test : testCases) {
    SECTION(test.name)
    {
      double actual_t;
      double actual_dist = calculateLineLineDistance(test.l1b, test.l1e, test.l2b, test.l2e, actual_t);

      CHECK(actual_dist == Catch::Approx(test.expected_dist).margin(epsilon));

      if (std::isnan(test.expected_t)) {
        CHECK(std::isnan(actual_t));
      } else {
        CHECK(actual_t == Catch::Approx(test.expected_t).margin(epsilon));
      }
    }
  }
}
