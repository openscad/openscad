#include "io/depthmap.h"

// Not implemented yet - the tests in depthmap_test.cc describe the intended
// behaviour and are expected to fail until this is written.
DepthImage encode_depthmap(const std::vector<float>& depths, std::uint32_t width,
                           std::uint32_t height, DepthProfile profile)
{
  DepthImage img;
  img.bytesPerPixel = profile == DepthProfile::metric ? 2 : 3;
  img.pixels.assign(static_cast<size_t>(width) * height * img.bytesPerPixel, 0);
  return img;
}
