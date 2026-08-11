#include "io/depthmap.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
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

std::vector<float> linearize_depth(const std::vector<float>& windowDepth, double clipNear,
                                   double clipFar, bool perspective)
{
  std::vector<float> mm;
  mm.reserve(windowDepth.size());

  for (const float z : windowDepth) {
    // Anything at (or past) the far plane is empty space, not a surface.
    if (!(z < 1.0f)) {
      mm.push_back(std::numeric_limits<float>::infinity());
      continue;
    }
    double eye;
    double zero;
    if (perspective) {
      // Undo the hyperbolic projection: z_ndc = 2z - 1, and
      // eye = 2nf / (f + n - z_ndc(f - n)).
      const double ndc = 2.0 * z - 1.0;
      const double denom = clipFar + clipNear - ndc * (clipFar - clipNear);
      if (denom == 0.0) {
        mm.push_back(std::numeric_limits<float>::infinity());
        continue;
      }
      eye = 2.0 * clipNear * clipFar / denom;
      // The perspective near plane is just in front of the scene (0.1*dist), so
      // it is a sensible origin.
      zero = clipNear;
    } else {
      // Orthographic depth is already linear in eye distance.
      eye = clipNear + z * (clipFar - clipNear);
      // ...but the orthographic near plane is 100*dist *behind* the eye, so
      // measuring from it would add a large offset unrelated to the model and
      // overflow the metric profile's ceiling for ordinary-sized models. Measure
      // from the eye instead. Geometry behind the eye stays negative rather than
      // being clamped here; the metric profile floors it at 0.
      zero = 0.0;
    }
    mm.push_back(static_cast<float>(eye - zero));
  }

  return mm;
}

DepthImage encode_depthmap(const std::vector<float>& depths, std::uint32_t width, std::uint32_t height,
                           DepthProfile profile)
{
  const size_t count = std::min(depths.size(), static_cast<size_t>(width) * height);

  DepthImage img;
  // Visual is RGBA rather than RGB because that is what write_png() consumes on
  // both backends - the CoreGraphics writer is kCGImageAlphaNoneSkipLast, and
  // lodepng's info_raw defaults to RGBA8. The alpha is dropped on the way out.
  img.bytesPerPixel = profile == DepthProfile::metric ? 2 : 4;
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
      img.pixels.push_back(255);  // opaque; write_png() discards it
    }
  }

  return img;
}
