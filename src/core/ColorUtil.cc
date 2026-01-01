#include <boost/spirit/home/support/common_terminals.hpp>
#include <string>
#include <unordered_map>
#include "geometry/linalg.h"
#include "core/ColorUtil.h"
#include "utils/printutils.h"
#include <boost/algorithm/string/case_conv.hpp>

#include "core/WebColors.h"
#include "core/XkcdColors.h"

namespace {

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

// Parses hex colors according to: https://drafts.csswg.org/css-color/#typedef-hex-color.
// If the input is invalid, returns boost::none.
// Supports the following formats:
// * "#rrggbb"
// * "#rrggbbaa"
// * "#rgb"
// * "#rgba"
std::optional<Color4f> parse_hex_color(const std::string& hex)
{
  // validate size. short syntax uses one hex digit per color channel instead of 2.
  const bool short_syntax = hex.size() == 4 || hex.size() == 5;
  const bool long_syntax = hex.size() == 7 || hex.size() == 9;
  if (!short_syntax && !long_syntax) return {};

  // validate
  if (hex[0] != '#') return {};
  if (!std::all_of(std::begin(hex) + 1, std::end(hex),
                   [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); })) {
    return {};
  }

  // number of characters per color channel
  const int stride = short_syntax ? 1 : 2;
  const float channel_max = short_syntax ? 15.0f : 255.0f;

  Vector4f rgba;
  rgba[3] = 1.0;  // default alpha to 100%

  for (unsigned i = 0; i < (hex.size() - 1) / stride; ++i) {
    const std::string chunk = hex.substr(1 + i * stride, stride);

    // convert the hex character(s) to a fraction between 0 and 1, inclusive
    rgba[i] = stoi(chunk, nullptr, 16) / channel_max;
  }

  return rgba;
}

std::optional<Color4f> parse_named_color(const std::string& col)
{
  const auto colorname = boost::algorithm::to_lower_copy(col);
  if (OpenSCAD::web_colors.find(colorname) != OpenSCAD::web_colors.end()) {
    return OpenSCAD::web_colors.at(colorname);
  }
  const auto xkcdname = colorname.rfind("xkcd:", 0) == 0 ? colorname.substr(5) : colorname;
  if (OpenSCAD::xkcd_colors.find(xkcdname) != OpenSCAD::xkcd_colors.end()) {
    return OpenSCAD::xkcd_colors.at(xkcdname);
  }
  return {};
}

}  // namespace

namespace OpenSCAD {

std::optional<Color4f> parse_color(const std::string& col)
{
  auto webcolor = ::parse_named_color(col);
  if (webcolor) {
    return webcolor;
  }

  auto hexcolor = ::parse_hex_color(col);
  if (hexcolor) {
    return hexcolor;
  }

  return {};
}

Color4f getColor(const std::string& col, const Color4f& defaultcolor)
{
  const auto parsed = parse_color(col);

  if (!parsed) {
    LOG(message_group::Warning, "Unable to parse color \"%1$s\", reverting to default color.", col);
  }

  return parsed.value_or(defaultcolor);
}

Vector4f getColorHSV(const Color4f& col)
{
  float h, s, v;
  ::rgbtohsv(col.r(), col.g(), col.b(), h, s, v);
  return {h, s, v, col.a()};
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
  const auto hsv = getColorHSV(col);
  float Y = 0.2126f * col.r() + 0.7152f * col.g() + 0.0722f * col.b();
  float S = hsv[1];

  if (S < 0.5) {
    // low saturation, choose between black / white based on luminance Y
    const float val = Y > 0.5 ? 0.0f : 1.0f;
    return {val, val, val, 1.0f};
  } else {
    float H = 360 * hsv[0];
    if ((H < 60) || (H > 300)) {
      return {0.0f, 1.0f, 1.0f, 1.0f};  // red -> cyan
    } else if (H < 180) {
      return {1.0f, 0.0f, 1.0f, 1.0f};  // green -> magenta
    } else {
      return {1.0f, 1.0f, 0.0f, 1.0f};  // blue -> yellow
    }
  }
}

ColorMaps getColorMaps()
{
  ColorMaps colorMaps{
    {COLOR_MAP_NAME_WEB_COLORS, web_colors},
    {COLOR_MAP_NAME_XKCD_COLORS, xkcd_colors},
  };
  return colorMaps;
}

}  // namespace OpenSCAD
