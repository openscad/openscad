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
  }

  SECTION("NoColor (-1) is the legacy missing-color sentinel")
  {
    color_index_t c(-1);
    REQUIRE(c.isNoColor());
    REQUIRE_FALSE(c.index().has_value());
  }

  SECTION("a sentinel can never be mistaken for an actual array index")
  {
    for (int32_t raw : {color_index_t::kNoColor, -7, -100}) {
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
