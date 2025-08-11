#include <boost/spirit/home/support/common_terminals.hpp>
#include <string>
#include <unordered_map>
#include "geometry/linalg.h"
#include "core/ColorUtil.h"
#include "utils/printutils.h"
#include <boost/algorithm/string/case_conv.hpp>

namespace {

// Colors extracted from https://drafts.csswg.org/css-color/ on 2015-08-02
// CSS Color Module Level 4 - Editor’s Draft, 29 May 2015
std::unordered_map<std::string, Color4f> webcolors{{"aliceblue", {240, 248, 255}},
                                                   {"antiquewhite", {250, 235, 215}},
                                                   {"aqua", {0, 255, 255}},
                                                   {"aquamarine", {127, 255, 212}},
                                                   {"azure", {240, 255, 255}},
                                                   {"beige", {245, 245, 220}},
                                                   {"bisque", {255, 228, 196}},
                                                   {"black", {0, 0, 0}},
                                                   {"blanchedalmond", {255, 235, 205}},
                                                   {"blue", {0, 0, 255}},
                                                   {"blueviolet", {138, 43, 226}},
                                                   {"brown", {165, 42, 42}},
                                                   {"burlywood", {222, 184, 135}},
                                                   {"cadetblue", {95, 158, 160}},
                                                   {"chartreuse", {127, 255, 0}},
                                                   {"chocolate", {210, 105, 30}},
                                                   {"coral", {255, 127, 80}},
                                                   {"cornflowerblue", {100, 149, 237}},
                                                   {"cornsilk", {255, 248, 220}},
                                                   {"crimson", {220, 20, 60}},
                                                   {"cyan", {0, 255, 255}},
                                                   {"darkblue", {0, 0, 139}},
                                                   {"darkcyan", {0, 139, 139}},
                                                   {"darkgoldenrod", {184, 134, 11}},
                                                   {"darkgray", {169, 169, 169}},
                                                   {"darkgreen", {0, 100, 0}},
                                                   {"darkgrey", {169, 169, 169}},
                                                   {"darkkhaki", {189, 183, 107}},
                                                   {"darkmagenta", {139, 0, 139}},
                                                   {"darkolivegreen", {85, 107, 47}},
                                                   {"darkorange", {255, 140, 0}},
                                                   {"darkorchid", {153, 50, 204}},
                                                   {"darkred", {139, 0, 0}},
                                                   {"darksalmon", {233, 150, 122}},
                                                   {"darkseagreen", {143, 188, 143}},
                                                   {"darkslateblue", {72, 61, 139}},
                                                   {"darkslategray", {47, 79, 79}},
                                                   {"darkslategrey", {47, 79, 79}},
                                                   {"darkturquoise", {0, 206, 209}},
                                                   {"darkviolet", {148, 0, 211}},
                                                   {"deeppink", {255, 20, 147}},
                                                   {"deepskyblue", {0, 191, 255}},
                                                   {"dimgray", {105, 105, 105}},
                                                   {"dimgrey", {105, 105, 105}},
                                                   {"dodgerblue", {30, 144, 255}},
                                                   {"firebrick", {178, 34, 34}},
                                                   {"floralwhite", {255, 250, 240}},
                                                   {"forestgreen", {34, 139, 34}},
                                                   {"fuchsia", {255, 0, 255}},
                                                   {"gainsboro", {220, 220, 220}},
                                                   {"ghostwhite", {248, 248, 255}},
                                                   {"gold", {255, 215, 0}},
                                                   {"goldenrod", {218, 165, 32}},
                                                   {"gray", {128, 128, 128}},
                                                   {"green", {0, 128, 0}},
                                                   {"greenyellow", {173, 255, 47}},
                                                   {"grey", {128, 128, 128}},
                                                   {"honeydew", {240, 255, 240}},
                                                   {"hotpink", {255, 105, 180}},
                                                   {"indianred", {205, 92, 92}},
                                                   {"indigo", {75, 0, 130}},
                                                   {"ivory", {255, 255, 240}},
                                                   {"khaki", {240, 230, 140}},
                                                   {"lavender", {230, 230, 250}},
                                                   {"lavenderblush", {255, 240, 245}},
                                                   {"lawngreen", {124, 252, 0}},
                                                   {"lemonchiffon", {255, 250, 205}},
                                                   {"lightblue", {173, 216, 230}},
                                                   {"lightcoral", {240, 128, 128}},
                                                   {"lightcyan", {224, 255, 255}},
                                                   {"lightgoldenrodyellow", {250, 250, 210}},
                                                   {"lightgray", {211, 211, 211}},
                                                   {"lightgreen", {144, 238, 144}},
                                                   {"lightgrey", {211, 211, 211}},
                                                   {"lightpink", {255, 182, 193}},
                                                   {"lightsalmon", {255, 160, 122}},
                                                   {"lightseagreen", {32, 178, 170}},
                                                   {"lightskyblue", {135, 206, 250}},
                                                   {"lightslategray", {119, 136, 153}},
                                                   {"lightslategrey", {119, 136, 153}},
                                                   {"lightsteelblue", {176, 196, 222}},
                                                   {"lightyellow", {255, 255, 224}},
                                                   {"lime", {0, 255, 0}},
                                                   {"limegreen", {50, 205, 50}},
                                                   {"linen", {250, 240, 230}},
                                                   {"magenta", {255, 0, 255}},
                                                   {"maroon", {128, 0, 0}},
                                                   {"mediumaquamarine", {102, 205, 170}},
                                                   {"mediumblue", {0, 0, 205}},
                                                   {"mediumorchid", {186, 85, 211}},
                                                   {"mediumpurple", {147, 112, 219}},
                                                   {"mediumseagreen", {60, 179, 113}},
                                                   {"mediumslateblue", {123, 104, 238}},
                                                   {"mediumspringgreen", {0, 250, 154}},
                                                   {"mediumturquoise", {72, 209, 204}},
                                                   {"mediumvioletred", {199, 21, 133}},
                                                   {"midnightblue", {25, 25, 112}},
                                                   {"mintcream", {245, 255, 250}},
                                                   {"mistyrose", {255, 228, 225}},
                                                   {"moccasin", {255, 228, 181}},
                                                   {"navajowhite", {255, 222, 173}},
                                                   {"navy", {0, 0, 128}},
                                                   {"oldlace", {253, 245, 230}},
                                                   {"olive", {128, 128, 0}},
                                                   {"olivedrab", {107, 142, 35}},
                                                   {"orange", {255, 165, 0}},
                                                   {"orangered", {255, 69, 0}},
                                                   {"orchid", {218, 112, 214}},
                                                   {"palegoldenrod", {238, 232, 170}},
                                                   {"palegreen", {152, 251, 152}},
                                                   {"paleturquoise", {175, 238, 238}},
                                                   {"palevioletred", {219, 112, 147}},
                                                   {"papayawhip", {255, 239, 213}},
                                                   {"peachpuff", {255, 218, 185}},
                                                   {"peru", {205, 133, 63}},
                                                   {"pink", {255, 192, 203}},
                                                   {"plum", {221, 160, 221}},
                                                   {"powderblue", {176, 224, 230}},
                                                   {"purple", {128, 0, 128}},
                                                   {"rebeccapurple", {102, 51, 153}},
                                                   {"red", {255, 0, 0}},
                                                   {"rosybrown", {188, 143, 143}},
                                                   {"royalblue", {65, 105, 225}},
                                                   {"saddlebrown", {139, 69, 19}},
                                                   {"salmon", {250, 128, 114}},
                                                   {"sandybrown", {244, 164, 96}},
                                                   {"seagreen", {46, 139, 87}},
                                                   {"seashell", {255, 245, 238}},
                                                   {"sienna", {160, 82, 45}},
                                                   {"silver", {192, 192, 192}},
                                                   {"skyblue", {135, 206, 235}},
                                                   {"slateblue", {106, 90, 205}},
                                                   {"slategray", {112, 128, 144}},
                                                   {"slategrey", {112, 128, 144}},
                                                   {"snow", {255, 250, 250}},
                                                   {"springgreen", {0, 255, 127}},
                                                   {"steelblue", {70, 130, 180}},
                                                   {"tan", {210, 180, 140}},
                                                   {"teal", {0, 128, 128}},
                                                   {"thistle", {216, 191, 216}},
                                                   {"tomato", {255, 99, 71}},
                                                   {"turquoise", {64, 224, 208}},
                                                   {"violet", {238, 130, 238}},
                                                   {"wheat", {245, 222, 179}},
                                                   {"white", {255, 255, 255}},
                                                   {"whitesmoke", {245, 245, 245}},
                                                   {"yellow", {255, 255, 0}},
                                                   {"yellowgreen", {154, 205, 50}},

                                                   // additional OpenSCAD specific entry
                                                   {"transparent", {0, 0, 0, 0}}};

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

std::optional<Color4f> parse_web_color(const std::string& col)
{
  std::string colorname = boost::algorithm::to_lower_copy(col);
  if (webcolors.find(colorname) != webcolors.end()) {
    return webcolors.at(colorname);
  }
  return {};
}

}  // namespace

namespace OpenSCAD {

std::optional<Color4f> parse_color(const std::string& col)
{
  auto webcolor = ::parse_web_color(col);
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
    LOG("Please see https://en.wikipedia.org/wiki/Web_colors");
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

}  // namespace OpenSCAD
