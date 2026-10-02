#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "io/imageutils.h"
#include "lodepng/lodepng.h"

namespace {

// A 2x2 RGBA image: opaque red, opaque green, fully transparent blue, half-transparent white.
std::vector<uint8_t> testPixels()
{
  return {
    255, 0,   0,   255,  //
    0,   255, 0,   255,  //
    0,   0,   255, 0,    //
    255, 255, 255, 128,  //
  };
}

// Encode via write_png(), then decode back with lodepng, forcing the decoder to report whatever
// colortype the file actually declares rather than converting it for us.
lodepng::State decodePng(const std::string& png, std::vector<uint8_t>& out, unsigned& w, unsigned& h)
{
  lodepng::State state;
  state.decoder.color_convert = 0;
  const std::vector<uint8_t> in(png.begin(), png.end());
  REQUIRE(lodepng::decode(out, w, h, state, in) == 0);
  return state;
}

std::string encode(bool with_alpha)
{
  auto pixels = testPixels();
  std::ostringstream out(std::ios::binary);
  REQUIRE(write_png(out, pixels.data(), 2, 2, with_alpha));
  return out.str();
}

}  // namespace

TEST_CASE("write_png discards alpha by default", "[imageutils]")
{
  std::vector<uint8_t> decoded;
  unsigned w = 0, h = 0;
  auto state = decodePng(encode(false), decoded, w, h);

  CHECK(w == 2);
  CHECK(h == 2);
  // No alpha channel at all, so nothing downstream can misinterpret it.
  CHECK(state.info_png.color.colortype == LCT_RGB);
  REQUIRE(decoded.size() == 2 * 2 * 3);
  // Colors survive, including the one whose source alpha was 0.
  CHECK(decoded[0] == 255);  // red
  CHECK(decoded[1] == 0);
  CHECK(decoded[2] == 0);
  CHECK(decoded[6] == 0);  // blue, was fully transparent
  CHECK(decoded[7] == 0);
  CHECK(decoded[8] == 255);
}

TEST_CASE("write_png preserves alpha when asked", "[imageutils]")
{
  std::vector<uint8_t> decoded;
  unsigned w = 0, h = 0;
  auto state = decodePng(encode(true), decoded, w, h);

  CHECK(w == 2);
  CHECK(h == 2);
  CHECK(state.info_png.color.colortype == LCT_RGBA);
  REQUIRE(decoded.size() == 2 * 2 * 4);
  CHECK(decoded[3] == 255);   // opaque red stays opaque
  CHECK(decoded[11] == 0);    // fully transparent pixel stays transparent
  CHECK(decoded[15] == 128);  // partial alpha is not rounded to 0 or 255
  // Alpha must not be premultiplied into the color channels.
  CHECK(decoded[12] == 255);
  CHECK(decoded[13] == 255);
  CHECK(decoded[14] == 255);
}
