#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "io/imageutils.h"
#include "lodepng/lodepng.h"

namespace {

// A 2x2 RGBA image. write_png() writes RGB, so the alpha values here are the ones it must drop.
std::vector<uint8_t> testPixels()
{
  return {
    255, 0,   0,   255,  //
    0,   255, 0,   255,  //
    0,   0,   255, 0,    //
    255, 255, 255, 128,  //
  };
}

std::string encode()
{
  auto pixels = testPixels();
  std::ostringstream out(std::ios::binary);
  REQUIRE(write_png(out, pixels.data(), 2, 2));
  return out.str();
}

// PNG is an 8-byte signature followed by length/type/data/crc chunks.
bool hasChunk(const std::string& png, const std::string& type)
{
  for (size_t i = 8; i + 8 <= png.size();) {
    const uint32_t len = (static_cast<uint8_t>(png[i]) << 24) |
                         (static_cast<uint8_t>(png[i + 1]) << 16) |
                         (static_cast<uint8_t>(png[i + 2]) << 8) | static_cast<uint8_t>(png[i + 3]);
    if (png.substr(i + 4, 4) == type) return true;
    i += 12 + len;
  }
  return false;
}

}  // namespace

TEST_CASE("write_png writes RGB and keeps the colors", "[imageutils]")
{
  std::vector<uint8_t> decoded;
  unsigned w = 0, h = 0;
  lodepng::State state;
  // Report the colortype the file actually declares rather than converting it for us.
  state.decoder.color_convert = 0;
  const auto png = encode();
  const std::vector<uint8_t> in(png.begin(), png.end());
  REQUIRE(lodepng::decode(decoded, w, h, state, in) == 0);

  CHECK(w == 2);
  CHECK(h == 2);
  CHECK(state.info_png.color.colortype == LCT_RGB);
  REQUIRE(decoded.size() == 2 * 2 * 3);
  CHECK(decoded[0] == 255);  // red
  CHECK(decoded[1] == 0);
  CHECK(decoded[2] == 0);
  CHECK(decoded[6] == 0);  // blue, whose source alpha was 0, is unaffected by dropping alpha
  CHECK(decoded[7] == 0);
  CHECK(decoded[8] == 255);
}

// The same render exported on two platforms should produce the same file. An embedded color
// profile is the difference that survived having a second, platform-specific encoder.
TEST_CASE("write_png embeds no color profile", "[imageutils]")
{
  const auto png = encode();
  CHECK_FALSE(hasChunk(png, "iCCP"));
  CHECK_FALSE(hasChunk(png, "sRGB"));
  CHECK_FALSE(hasChunk(png, "gAMA"));
  CHECK_FALSE(hasChunk(png, "cHRM"));
}
