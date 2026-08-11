#include "io/depthmap.h"

#include <catch2/catch_all.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

constexpr float BG = std::numeric_limits<float>::infinity();

std::uint16_t grey16(const DepthImage& img, size_t pixel)
{
  // PNG stores 16-bit samples big-endian.
  return static_cast<std::uint16_t>(img.pixels[pixel * 2] << 8 | img.pixels[pixel * 2 + 1]);
}

}  // namespace

TEST_CASE("metric profile encodes linear millimetres, near dark", "[Depthmap]")
{
  // A 2x2 buffer: three depths plus one background pixel.
  const std::vector<float> depths = {10.0f, 20.0f, 100.0f, BG};
  const auto img = encode_depthmap(depths, 2, 2, DepthProfile::metric);

  REQUIRE(img.bytesPerPixel == 2);
  REQUIRE(img.pixels.size() == 4 * 2);

  // Value is the distance itself, so it survives a round trip to millimetres.
  CHECK(grey16(img, 0) == 10);
  CHECK(grey16(img, 1) == 20);
  CHECK(grey16(img, 2) == 100);
  // Background is the far end, and is not confusable with a near surface.
  CHECK(grey16(img, 3) == 65535);

  CHECK(img.minDepth == Catch::Approx(10.0));
  CHECK(img.maxDepth == Catch::Approx(100.0));
}

TEST_CASE("metric profile clamps beyond the representable range", "[Depthmap]")
{
  const std::vector<float> depths = {-5.0f, 70000.0f};
  const auto img = encode_depthmap(depths, 2, 1, DepthProfile::metric);

  // Nothing wraps: a negative depth floors at 0, an overlong one saturates
  // just below the background value so it stays distinguishable from no-data.
  CHECK(grey16(img, 0) == 0);
  CHECK(grey16(img, 1) == 65534);
}

TEST_CASE("visual profile normalizes to the model extent, near bright", "[Depthmap]")
{
  const std::vector<float> depths = {10.0f, 20.0f, 30.0f, BG};
  const auto img = encode_depthmap(depths, 2, 2, DepthProfile::visual);

  // RGBA, because that is what write_png() consumes on both backends: the
  // CoreGraphics writer is kCGImageAlphaNoneSkipLast and lodepng's info_raw
  // defaults to RGBA8. Alpha is dropped on the way out to a 3-channel PNG.
  REQUIRE(img.bytesPerPixel == 4);
  REQUIRE(img.pixels.size() == 4 * 4);

  // Nearest surface saturates white, farthest goes black, and the range is
  // stretched across the model's own extent rather than the clip planes.
  CHECK(img.pixels[0] == 255);
  CHECK(img.pixels[4] == 128);
  CHECK(img.pixels[8] == 0);
  // Background is black too - indistinguishable from the far surface, which is
  // what MiDaS-trained consumers expect.
  CHECK(img.pixels[12] == 0);

  // Grey, so a consumer reading only one channel gets the same answer.
  CHECK(img.pixels[0] == img.pixels[1]);
  CHECK(img.pixels[1] == img.pixels[2]);
  CHECK(img.pixels[3] == 255);  // opaque

  CHECK(img.minDepth == Catch::Approx(10.0));
  CHECK(img.maxDepth == Catch::Approx(30.0));
}

TEST_CASE("visual profile handles a model of zero depth extent", "[Depthmap]")
{
  // A flat face perpendicular to the view: min == max, so normalizing would
  // divide by zero.
  const std::vector<float> depths = {42.0f, 42.0f, BG, BG};
  const auto img = encode_depthmap(depths, 2, 2, DepthProfile::visual);

  CHECK(img.pixels[0] == 255);
  CHECK(img.pixels[4] == 255);
  CHECK(img.pixels[8] == 0);
}

TEST_CASE("orthographic depth is measured from the eye, not the near plane", "[Depthmap]")
{
  // Near/far here are the -100*dist/+100*dist that GLView::setupCamera uses for
  // an orthographic camera. That near plane sits 100*dist *behind* the eye, so
  // measuring from it would offset every value by a distance that has nothing to
  // do with the model - and would overflow the metric profile's 65535mm ceiling
  // for any model over roughly 328 units. Orthographic depth is therefore
  // reported from the eye, where the numbers mean something.
  const std::vector<float> window = {0.5f, 0.75f, 0.9f};
  const auto mm = linearize_depth(window, -100.0, 100.0, false);

  REQUIRE(mm.size() == 3);
  CHECK(mm[0] == Catch::Approx(0.0));   // halfway through the clip range is the eye
  CHECK(mm[1] == Catch::Approx(50.0));  // linear in eye distance, so no curve to undo
  CHECK(mm[2] == Catch::Approx(80.0));
}

TEST_CASE("orthographic geometry behind the eye reads as negative", "[Depthmap]")
{
  // The orthographic near plane is behind the eye, so the clip volume genuinely
  // contains negative eye distances. They are left negative here rather than
  // clamped, so the caller can see them; the metric profile floors them at 0.
  const auto mm = linearize_depth({0.25f}, -100.0, 100.0, false);
  CHECK(mm[0] == Catch::Approx(-50.0));
}

TEST_CASE("perspective depth unprojects through the hyperbolic curve", "[Depthmap]")
{
  // n=1, f=101. A surface at eye distance d has window depth
  // f*(d-n) / ((f-n)*d), so d=2 gives 101/200 = 0.505.
  const std::vector<float> window = {0.0f, 0.505f};
  const auto mm = linearize_depth(window, 1.0, 101.0, true);

  REQUIRE(mm.size() == 2);
  CHECK(mm[0] == Catch::Approx(0.0));  // near plane, so zero distance from it
  // Eye distance 2, minus the near plane. The margin is loose because window
  // depth arrives as a float and the curve is steep here - which is the
  // precision hazard of a wide near/far ratio, visible even in this toy case.
  CHECK(mm[1] == Catch::Approx(1.0).margin(1e-4));
}

TEST_CASE("the far plane is background, not a real distance", "[Depthmap]")
{
  const std::vector<float> window = {1.0f, 0.5f};

  const auto ortho = linearize_depth(window, -100.0, 100.0, false);
  CHECK(std::isinf(ortho[0]));
  CHECK(std::isfinite(ortho[1]));

  const auto persp = linearize_depth(window, 1.0, 101.0, true);
  CHECK(std::isinf(persp[0]));
  CHECK(std::isfinite(persp[1]));
}

TEST_CASE("the viewport depth range spans the bounding box extent", "[Depthmap]")
{
  // Taken from the model's own eye-space extent, not from what is on screen, so
  // rotating the model does not change the shading of a surface that did not move.
  const auto r = depth_range_for_bounds(90.0, 110.0);
  CHECK(r.start == Catch::Approx(90.0));
  CHECK(r.end == Catch::Approx(110.0));
}

TEST_CASE("the viewport depth range survives the camera being inside the model", "[Depthmap]")
{
  // Zoomed in past the model's near side: the near end would go negative, which
  // fog will not accept, so it floors at zero without collapsing the range.
  const auto r = depth_range_for_bounds(-5.0, 15.0);
  CHECK(r.start == Catch::Approx(0.0));
  CHECK(r.end == Catch::Approx(15.0));
  CHECK(r.end > r.start);
}

TEST_CASE("the viewport depth range never collapses to a point", "[Depthmap]")
{
  // A zero-radius bound (a single point, or a degenerate model) would make fog
  // divide by zero.
  const auto r = depth_range_for_bounds(50.0, 50.0);
  CHECK(r.end > r.start);
}

TEST_CASE("an empty view yields background everywhere", "[Depthmap]")
{
  const std::vector<float> depths = {BG, BG};

  const auto metric = encode_depthmap(depths, 2, 1, DepthProfile::metric);
  CHECK(grey16(metric, 0) == 65535);
  CHECK(grey16(metric, 1) == 65535);
  CHECK(metric.minDepth == 0.0);
  CHECK(metric.maxDepth == 0.0);

  const auto visual = encode_depthmap(depths, 2, 1, DepthProfile::visual);
  CHECK(visual.pixels[0] == 0);
  CHECK(visual.pixels[4] == 0);
}
