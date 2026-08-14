#include <catch2/catch_test_macros.hpp>
#include "glview/ColorMap.h"
#include <filesystem>

TEST_CASE("RenderColorScheme parses 8-digit hex colors with alpha", "[colormap]")
{
  std::filesystem::path schemePath = "color-schemes/render/transparent.json";
  RenderColorScheme scheme(schemePath);
  REQUIRE(scheme.valid());

  auto bgcol = ColorMap::getColor(scheme.colorScheme(), RenderColor::BACKGROUND_COLOR);
  int r = 0, g = 0, b = 0, a = 0;
  REQUIRE(bgcol.getRgba(r, g, b, a));
  CHECK(r == 0x33);
  CHECK(g == 0x33);
  CHECK(b == 0x33);
  CHECK(a == 0);
}
