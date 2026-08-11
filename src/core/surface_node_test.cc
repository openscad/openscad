#include "core/SurfaceNode.h"

#include <catch2/catch_all.hpp>
#include <filesystem>
#include <vector>

#include "lodepng/lodepng.h"

namespace fs = std::filesystem;

TEST_CASE("SurfaceNode 8-bit and 16-bit PNG import", "[core][SurfaceNode]")
{
  auto tmp_dir = fs::temp_directory_path() / "surface_node_test";
  fs::create_directories(tmp_dir);

  SECTION("8-bit PNG import preserves standard height scaling")
  {
    auto path_8bit = (tmp_dir / "test_8bit.png").string();

    // 2x2 8-bit grayscale image
    // pixels: 0, 85, 170, 255
    std::vector<unsigned char> img_8bit = {0, 85, 170, 255};
    std::vector<unsigned char> png_8bit;
    unsigned error = lodepng::encode(png_8bit, img_8bit, 2, 2, LCT_GREY, 8);
    REQUIRE(error == 0);
    lodepng::save_file(png_8bit, path_8bit);

    SurfaceNode node;
    img_data_t data = node.read_png_or_dat(path_8bit);

    REQUIRE(data.width == 2);
    REQUIRE(data.height == 2);

    // convert_image flips y: row 0 in data is bottom row of image (y=1)
    // y=1: x=0 -> 170, x=1 -> 255
    // y=0: x=0 -> 0,   x=1 -> 85
    CHECK(data[0] == Catch::Approx(100.0 / 255.0 * 170.0).margin(1e-4));
    CHECK(data[1] == Catch::Approx(100.0 / 255.0 * 255.0).margin(1e-4));
    CHECK(data[2] == Catch::Approx(100.0 / 255.0 * 0.0).margin(1e-4));
    CHECK(data[3] == Catch::Approx(100.0 / 255.0 * 85.0).margin(1e-4));

    fs::remove(path_8bit);
  }

  SECTION("16-bit PNG import preserves 16-bit depth precision")
  {
    auto path_16bit = (tmp_dir / "test_16bit.png").string();

    // 2x1 16-bit grayscale image with values 32768 and 32769 (differing by 1 in 16-bit space)
    // lodepng expects 16-bit raw values in big-endian (MSB first)
    uint16_t v0 = 32768;
    uint16_t v1 = 32769;

    std::vector<unsigned char> img_16bit = {
      static_cast<unsigned char>((v0 >> 8) & 0xFF), static_cast<unsigned char>(v0 & 0xFF),
      static_cast<unsigned char>((v1 >> 8) & 0xFF), static_cast<unsigned char>(v1 & 0xFF)};

    std::vector<unsigned char> png_16bit;
    unsigned error = lodepng::encode(png_16bit, img_16bit, 2, 1, LCT_GREY, 16);
    REQUIRE(error == 0);
    lodepng::save_file(png_16bit, path_16bit);

    SurfaceNode node;
    img_data_t data = node.read_png_or_dat(path_16bit);

    REQUIRE(data.width == 2);
    REQUIRE(data.height == 1);

    double expected_z0 = 100.0 / 65535.0 * 32768.0;
    double expected_z1 = 100.0 / 65535.0 * 32769.0;

    // Verify 16-bit precision: z0 and z1 must be distinct and match 16-bit scaling
    CHECK(data[0] == Catch::Approx(expected_z0).margin(1e-6));
    CHECK(data[1] == Catch::Approx(expected_z1).margin(1e-6));
    CHECK(data[1] > data[0]);

    fs::remove(path_16bit);
  }

  fs::remove_all(tmp_dir);
}
