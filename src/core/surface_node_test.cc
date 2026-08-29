#include "core/SurfaceNode.h"

#include <catch2/catch_all.hpp>
#include <vector>

static std::string get_test_image(const std::string& name)
{
  return std::string(OPENSCAD_TEST_DATA_DIR) + "/image/" + name;
}

TEST_CASE("SurfaceNode 8-bit and 16-bit PNG import", "[core][SurfaceNode]")
{
  SECTION("8-bit PNG import preserves standard height scaling")
  {
    auto path_8bit = get_test_image("surface_test_8bit.png");

    SurfaceNode node;
    img_data_t data = node.read_png_or_dat(path_8bit);

    REQUIRE(data.width == 2);
    REQUIRE(data.height == 2);

    // convert_image flips y: row 0 in data is bottom row of image (y=1)
    // image pixels: [0, 85, 170, 255]
    // y=1: x=0 -> 170, x=1 -> 255
    // y=0: x=0 -> 0,   x=1 -> 85
    CHECK(data[0] == Catch::Approx(100.0 / 65535.0 * 43690.0).margin(1e-4));
    CHECK(data[1] == Catch::Approx(100.0 / 65535.0 * 65535.0).margin(1e-4));
    CHECK(data[2] == Catch::Approx(100.0 / 65535.0 * 0.0).margin(1e-4));
    CHECK(data[3] == Catch::Approx(100.0 / 65535.0 * 21845.0).margin(1e-4));
  }

  SECTION("16-bit PNG import preserves 16-bit depth precision")
  {
    auto path_16bit = get_test_image("surface_test_16bit.png");

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
  }

  SECTION("invert scales identically for 8-bit and 16-bit inputs")
  {
    auto path_8bit = get_test_image("surface_test_invert_8bit.png");
    auto path_16bit = get_test_image("surface_test_invert_16bit.png");

    SurfaceNode node;
    node.invert = true;
    img_data_t data_8 = node.read_png_or_dat(path_8bit);
    img_data_t data_16 = node.read_png_or_dat(path_16bit);

    // invert must mirror about 0.0 to minimize breaking changes from the original 1.0 mirroring.
    // 8-bit image has pixel value 64. 64 -> 16448
    // 16-bit image has pixel value 16449.
    CHECK(data_8[0] == Catch::Approx(100.0 / 65535.0 * (0.0 - 16448.0)).margin(1e-4));
    CHECK(data_16[0] == Catch::Approx(100.0 / 65535.0 * (0.0 - 16449.0)).margin(1e-4));
  }
}
