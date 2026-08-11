#include "io/depthmap.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

//! Reserved for pixels with no geometry, so a saturating real depth must stop
//! one short of it.
constexpr std::uint16_t METRIC_BACKGROUND = 65535;

void push_be16(std::vector<std::uint8_t>& pixels, std::uint16_t value)
{
  // PNG stores 16-bit samples big-endian.
  pixels.push_back(static_cast<std::uint8_t>(value >> 8));
  pixels.push_back(static_cast<std::uint8_t>(value & 0xff));
}

}  // namespace

DepthImage encode_depthmap(const std::vector<float>& depths, std::uint32_t width, std::uint32_t height,
                           DepthProfile profile)
{
  const size_t count = std::min(depths.size(), static_cast<size_t>(width) * height);

  DepthImage img;
  img.bytesPerPixel = profile == DepthProfile::metric ? 2 : 3;
  img.pixels.reserve(count * img.bytesPerPixel);

  bool any = false;
  for (size_t i = 0; i < count; ++i) {
    if (!std::isfinite(depths[i])) continue;
    const double d = depths[i];
    if (!any) {
      img.minDepth = img.maxDepth = d;
      any = true;
    } else {
      img.minDepth = std::min(img.minDepth, d);
      img.maxDepth = std::max(img.maxDepth, d);
    }
  }
  if (!any) img.minDepth = img.maxDepth = 0.0;

  if (profile == DepthProfile::metric) {
    for (size_t i = 0; i < count; ++i) {
      if (!std::isfinite(depths[i])) {
        push_be16(img.pixels, METRIC_BACKGROUND);
        continue;
      }
      const double scaled = std::lround(depths[i] * DEPTHMAP_METRIC_SCALE);
      // Clamp rather than let the cast wrap: a model deeper than ~65m saturates
      // just below the background value so it stays distinguishable from no-data.
      const double clamped = std::clamp(scaled, 0.0, static_cast<double>(METRIC_BACKGROUND - 1));
      push_be16(img.pixels, static_cast<std::uint16_t>(clamped));
    }
  } else {
    // Normalizing over a zero extent would divide by zero - a face perpendicular
    // to the view is a single depth, and the whole of it is "nearest".
    const double extent = img.maxDepth - img.minDepth;
    for (size_t i = 0; i < count; ++i) {
      std::uint8_t grey = 0;  // background is black, matching MiDaS-shaped output
      if (std::isfinite(depths[i])) {
        const double t = extent > 0.0 ? (img.maxDepth - depths[i]) / extent : 1.0;
        grey = static_cast<std::uint8_t>(std::lround(t * 255.0));
      }
      img.pixels.insert(img.pixels.end(), 3, grey);
    }
  }

  return img;
}
