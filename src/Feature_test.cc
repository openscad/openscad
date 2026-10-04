#include "Feature.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Feature enable/disable round-trips and features() lists names", "[Feature]")
{
  const Feature& roof = Feature::ExperimentalRoof;

  const bool original = roof.is_enabled();
  Feature::enable_feature("roof", true);
  CHECK(roof.is_enabled());
  CHECK(roof.get_name() == "roof");

  Feature::enable_feature("roof", false);
  CHECK_FALSE(roof.is_enabled());

  CHECK(Feature::features().find("roof") != std::string::npos);

  // Restore whatever state existed before this test ran, since Feature state is static/global.
  Feature::enable_feature("roof", original);
}
