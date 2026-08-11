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

  REQUIRE(img.bytesPerPixel == 3);
  REQUIRE(img.pixels.size() == 4 * 3);

  // Nearest surface saturates white, farthest goes black, and the range is
  // stretched across the model's own extent rather than the clip planes.
  CHECK(img.pixels[0] == 255);
  CHECK(img.pixels[3] == 128);
  CHECK(img.pixels[6] == 0);
  // Background is black too - indistinguishable from the far surface, which is
  // what MiDaS-trained consumers expect.
  CHECK(img.pixels[9] == 0);

  // Grey, so a consumer reading only one channel gets the same answer.
  CHECK(img.pixels[0] == img.pixels[1]);
  CHECK(img.pixels[1] == img.pixels[2]);

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
  CHECK(img.pixels[3] == 255);
  CHECK(img.pixels[6] == 0);
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
  CHECK(visual.pixels[3] == 0);
}
