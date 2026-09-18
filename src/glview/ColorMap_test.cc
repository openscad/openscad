#include <catch2/catch_test_macros.hpp>
#include "glview/ColorMap.h"
#include <filesystem>
#include <fstream>

TEST_CASE("RenderColorScheme parses 8-digit hex colors with alpha", "[colormap]")
{
  namespace fs = std::filesystem;
  fs::path tempPath = fs::temp_directory_path() / "test_transparent_scheme.json";

  {
    std::ofstream ofs(tempPath);
    ofs << R"({
      "name": "TestTransparent",
      "index": 9999,
      "show-in-gui": true,
      "colors": {
        "background": "#33333300",
        "axes-color": "#c1c1c1",
        "opencsg-face-front": "#eeeeee",
        "opencsg-face-back": "#0babc8",
        "cgal-face-front": "#eeeeee",
        "cgal-face-back": "#0babc8",
        "cgal-face-2d": "#9370db",
        "cgal-edge-front": "#0000ff",
        "cgal-edge-back": "#0000ff",
        "cgal-edge-2d": "#ff00ff",
        "crosshair": "#f0f0f0"
      }
    })";
  }

  RenderColorScheme scheme(tempPath);
  fs::remove(tempPath);

  REQUIRE(scheme.valid());

  auto bgcol = ColorMap::getColor(scheme.colorScheme(), RenderColor::BACKGROUND_COLOR);
  int r = 0, g = 0, b = 0, a = 0;
  REQUIRE(bgcol.getRgba(r, g, b, a));
  CHECK(r == 0x33);
  CHECK(g == 0x33);
  CHECK(b == 0x33);
  CHECK(a == 0);
}
