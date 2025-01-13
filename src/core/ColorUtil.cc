#include "core/ColorUtil.h"

namespace OpenSCAD {

// See http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
void rgbtohsv(float r, float g, float b, float& h, float& s, float& v)
{
  float K = 0.f;

  if (g < b) {
    std::swap(g, b);
    K = -1.f;
  }

  if (r < g) {
    std::swap(r, g);
    K = -2.f / 6.f - K;
  }

  float chroma = r - std::min(g, b);
  h = std::fabs(K + (g - b) / (6.f * chroma + 1e-20f));
  s = chroma / (r + 1e-20f);
  v = r;
}

Color4f getColorHSV(const Color4f& col)
{
  float h, s, v;
  OpenSCAD::rgbtohsv(col[0], col[1], col[2], h, s, v);
  return {h, s, v, col[3]};
}

/**
 * Calculate contrast color. Based on the article
 * http://gamedev.stackexchange.com/questions/38536/given-a-rgb-color-x-how-to-find-the-most-contrasting-color-y
 *
 * @param col the input color
 * @return a color with high contrast to the input color
 */
Color4f getContrastColor(const Color4f& col)
{
  Color4f hsv = getColorHSV(col);
  float Y = 0.2126f * col[0] + 0.7152f * col[1] + 0.0722f * col[2];
  float S = hsv[1];

  if (S < 0.5) {
    // low saturation, choose between black / white based on luminance Y
    float val = Y > 0.5 ? 0.0f : 1.0f;
    return {val, val, val, 1.0f};
  } else {
    float H = 360 * hsv[0];
    if ((H < 60) || (H > 300)) {
      return {0.0f, 1.0f, 1.0f, 1.0f}; // red -> cyan
    } else if (H < 180) {
      return {1.0f, 0.0f, 1.0f, 1.0f}; // green -> magenta
    } else {
      return {1.0f, 1.0f, 0.0f, 1.0f}; // blue -> yellow
    }
  }
}

// Parses hex colors according to: https://drafts.csswg.org/css-color/#typedef-hex-color.
// If the input is invalid, returns boost::none.
// Supports the following formats:
// * "#rrggbb"
// * "#rrggbbaa"
// * "#rgb"
// * "#rgba"
std::optional<Color4f> parse_hex_color(const std::string& hex) {
  // validate size. short syntax uses one hex digit per color channel instead of 2.
  const bool short_syntax = hex.size() == 4 || hex.size() == 5;
  const bool long_syntax = hex.size() == 7 || hex.size() == 9;
  if (!short_syntax && !long_syntax) return {};

  // validate
  if (hex[0] != '#') return {};
  if (!std::all_of(std::begin(hex) + 1, std::end(hex),
                   [](char c) {
    return std::isxdigit(static_cast<unsigned char>(c));
  })) {
    return {};
  }

  // number of characters per color channel
  const int stride = short_syntax ? 1 : 2;
  const float channel_max = short_syntax ? 15.0f : 255.0f;

  Color4f rgba;
  rgba[3] = 1.0; // default alpha to 100%

  for (unsigned i = 0; i < (hex.size() - 1) / stride; ++i) {
    const std::string chunk = hex.substr(1 + i * stride, stride);

    // convert the hex character(s) from base 16 to base 10
    rgba[i] = stoi(chunk, nullptr, 16) / channel_max;
  }

  return rgba;
}

} // namespace