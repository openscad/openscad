#include "geometry/ColorIndex.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("color_index_t sentinel semantics", "[ColorIndex]")
{
  SECTION("a real index carries its value and .index() returns it")
  {
    color_index_t c(3);
    REQUIRE(c.raw() == 3);
    REQUIRE(c.index().has_value());
    REQUIRE(*c.index() == 3u);
    REQUIRE_FALSE(c.isNoColor());
    REQUIRE_FALSE(c.isCutout());
    REQUIRE_FALSE(c.isDefault());
  }

  SECTION("NoColor (-1) is the legacy missing-color sentinel")
  {
    color_index_t c(-1);
    REQUIRE(c.isNoColor());
    REQUIRE_FALSE(c.index().has_value());
  }

  SECTION("Cutout (-2) and Default (-3) are distinct sentinels a missing value can't be confused with")
  {
    color_index_t cutout(color_index_t::kCutout);
    color_index_t deflt(color_index_t::kDefault);

    REQUIRE(cutout.isCutout());
    REQUIRE_FALSE(cutout.isDefault());
    REQUIRE_FALSE(cutout.isNoColor());
    REQUIRE_FALSE(cutout.index().has_value());

    REQUIRE(deflt.isDefault());
    REQUIRE_FALSE(deflt.isCutout());
    REQUIRE_FALSE(deflt.index().has_value());

    REQUIRE(cutout != deflt);
    REQUIRE(cutout != color_index_t(color_index_t::kNoColor));
  }

  SECTION("a sentinel can never be mistaken for an actual array index")
  {
    for (int32_t raw : {color_index_t::kNoColor, color_index_t::kCutout, color_index_t::kDefault}) {
      color_index_t c(raw);
      REQUIRE_FALSE(c.index().has_value());
    }
  }

  SECTION("default construction is NoColor")
  {
    color_index_t c;
    REQUIRE(c.isNoColor());
  }

  SECTION("implicit construction from int32_t, e.g. plain -1 literals used across call sites")
  {
    color_index_t c = -1;
    REQUIRE(c.isNoColor());
  }
}
